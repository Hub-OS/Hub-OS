use super::{BattleCallback, BattleSimulation, SharedBattleResources};
use crate::battle::Entity;
use crate::bindable::{Element, EntityId, Team};
use crate::lua_api::create_custom_tile_state_table;
use crate::packages::{PackageInfo, PackageNamespace};
use crate::render::{Animator, FrameTime};
use crate::resources::{AssetManager, BROKEN_LIFETIME, Globals, ResourcePaths};
use framework::prelude::{GameIO, Texture};
use packets::structures::PackageCategory;
use std::sync::Arc;

#[derive(PartialEq, Eq, Clone, Copy)]
pub enum TileStateAnimationSupport {
    Single,
    SingleFlip,
    Rows,
    RowsFlip,
    Team,
    TeamFlip,
    TeamRows,
    TeamRowsFlip,
    None,
}

impl TileStateAnimationSupport {
    pub fn from_animator(animator: &Animator) -> Self {
        if animator.has_state("DEFAULT_FLIPPED") {
            Self::SingleFlip
        } else if animator.has_state("DEFAULT") {
            Self::Single
        } else if animator.has_state("2_FLIPPED") {
            Self::RowsFlip
        } else if animator.has_state("2") {
            Self::Rows
        } else if animator.has_state("RED_FLIPPED") {
            Self::TeamFlip
        } else if animator.has_state("RED") {
            Self::Team
        } else if animator.has_state("RED_2_FLIPPED") {
            Self::TeamRowsFlip
        } else if animator.has_state("RED_2") {
            Self::TeamRows
        } else {
            Self::None
        }
    }

    fn supports_flipped(self) -> bool {
        matches!(
            self,
            Self::SingleFlip | Self::RowsFlip | Self::TeamFlip | Self::TeamRowsFlip
        )
    }

    pub fn animation_state(
        self,
        team: Team,
        row: usize,
        perspective_flipped: bool,
    ) -> &'static str {
        // many matches to avoid string creation

        debug_assert!((1..=3).contains(&row));

        let team = if perspective_flipped {
            team.opposite()
        } else {
            team
        };

        let flipped = self.supports_flipped() && perspective_flipped;

        match self {
            TileStateAnimationSupport::Single
            | TileStateAnimationSupport::SingleFlip
            | TileStateAnimationSupport::None => {
                if flipped {
                    "DEFAULT_FLIPPED"
                } else {
                    "DEFAULT"
                }
            }
            TileStateAnimationSupport::Rows | TileStateAnimationSupport::RowsFlip => {
                match (row, flipped) {
                    (1, false) => "1",
                    (1, true) => "1_FLIPPED",
                    (2, false) => "2",
                    (2, true) => "2_FLIPPED",
                    (3, false) => "3",
                    (3, true) => "3_FLIPPED",
                    _ => unreachable!(),
                }
            }
            TileStateAnimationSupport::Team | TileStateAnimationSupport::TeamFlip => {
                match (team, flipped) {
                    (Team::Red, false) => "RED",
                    (Team::Red, true) => "RED_FLIPPED",
                    (Team::Blue, false) => "BLUE",
                    (Team::Blue, true) => "BLUE_FLIPPED",
                    (_, false) => "OTHER",
                    (_, true) => "OTHER_FLIPPED",
                }
            }
            TileStateAnimationSupport::TeamRows | TileStateAnimationSupport::TeamRowsFlip => {
                match (team, row, flipped) {
                    (Team::Red, 1, false) => "RED_1",
                    (Team::Red, 2, false) => "RED_2",
                    (Team::Red, 3, false) => "RED_3",
                    (Team::Red, 1, true) => "RED_1_FLIPPED",
                    (Team::Red, 2, true) => "RED_2_FLIPPED",
                    (Team::Red, 3, true) => "RED_3_FLIPPED",
                    (Team::Blue, 1, false) => "BLUE_1",
                    (Team::Blue, 2, false) => "BLUE_2",
                    (Team::Blue, 3, false) => "BLUE_3",
                    (Team::Blue, 1, true) => "BLUE_1_FLIPPED",
                    (Team::Blue, 2, true) => "BLUE_2_FLIPPED",
                    (Team::Blue, 3, true) => "BLUE_3_FLIPPED",
                    (_, 1, false) => "OTHER_1",
                    (_, 2, false) => "OTHER_2",
                    (_, 3, false) => "OTHER_3",
                    (_, 1, true) => "OTHER_1_FLIPPED",
                    (_, 2, true) => "OTHER_2_FLIPPED",
                    (_, 3, true) => "OTHER_3_FLIPPED",
                    _ => unreachable!(),
                }
            }
        }
    }
}

#[derive(Clone)]
pub struct TileState {
    pub state_name: String,
    pub texture: Arc<Texture>,
    pub animator: Animator,
    pub animation_support: TileStateAnimationSupport,
    pub cleanser_element: Option<Element>,
    pub blocks_team_change: bool,
    pub hide_frame: bool,
    pub hide_body: bool,
    pub is_hole: bool,
    pub max_lifetime: Option<FrameTime>,
    pub replace_callback: BattleCallback<(i32, i32)>,
    pub can_replace_callback: BattleCallback<(i32, i32, usize), bool>,
    pub update_callback: BattleCallback<(i32, i32)>,
    pub entity_enter_callback: BattleCallback<EntityId>,
    pub entity_leave_callback: BattleCallback<(EntityId, i32, i32)>,
    pub entity_stop_callback: BattleCallback<(EntityId, i32, i32)>,
}

impl TileState {
    pub const VOID: usize = 0;
    pub const NORMAL: usize = 1;
    pub const HOLE: usize = 2;
    pub const CRACKED: usize = 3;
    pub const BROKEN: usize = 4;

    pub fn new(state_name: String, texture: Arc<Texture>, animator: Animator) -> Self {
        let animation_support = TileStateAnimationSupport::from_animator(&animator);

        Self {
            state_name,
            texture,
            animator,
            animation_support,
            cleanser_element: None,
            blocks_team_change: false,
            hide_frame: false,
            hide_body: false,
            is_hole: false,
            max_lifetime: None,
            can_replace_callback: BattleCallback::stub(true),
            replace_callback: BattleCallback::default(),
            update_callback: BattleCallback::default(),
            entity_enter_callback: BattleCallback::default(),
            entity_leave_callback: BattleCallback::default(),
            entity_stop_callback: BattleCallback::default(),
        }
    }

    pub fn create_registry(game_io: &GameIO) -> Vec<TileState> {
        let mut registry = Vec::new();

        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let texture = globals.assets.texture(game_io, ResourcePaths::BATTLE_TILES);

        // Void
        debug_assert_eq!(registry.len(), TileState::VOID);
        let mut void_state = TileState::new(String::from("Void"), texture.clone(), Animator::new());
        void_state.blocks_team_change = true;
        void_state.is_hole = true;
        void_state.hide_frame = true;
        void_state.hide_body = true;
        void_state.can_replace_callback = BattleCallback::stub(false);
        registry.push(void_state);

        // Normal
        debug_assert_eq!(registry.len(), TileState::NORMAL);
        let normal_state = TileState::new(String::from("Normal"), texture.clone(), Animator::new());
        registry.push(normal_state);

        // Hole
        debug_assert_eq!(registry.len(), TileState::HOLE);
        let mut hole_state =
            TileState::new(String::from("PermaHole"), texture.clone(), Animator::new());
        hole_state.can_replace_callback = BattleCallback::stub(false);
        hole_state.blocks_team_change = true;
        hole_state.hide_body = true;
        hole_state.is_hole = true;
        registry.push(hole_state);

        // Cracked
        debug_assert_eq!(registry.len(), TileState::CRACKED);
        let animator = Animator::load_new(assets, ResourcePaths::BATTLE_TILE_CRACKED_ANIMATION);
        let mut cracked_state = TileState::new(String::from("Cracked"), texture.clone(), animator);
        cracked_state.hide_frame = true;
        cracked_state.hide_body = true;

        cracked_state.entity_leave_callback = BattleCallback::new(
            |game_io, _, simulation, (entity_id, old_x, old_y): (EntityId, i32, i32)| {
                let entities = &mut simulation.entities;
                let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                    return;
                };

                if entity.ignore_negative_tile_effects {
                    return;
                }

                let Some(tile) = simulation.field.tile_at_mut((old_x, old_y)) else {
                    return;
                };

                if tile.reservations().is_empty() {
                    if !simulation.is_resimulation {
                        let globals = Globals::from_resources(game_io);
                        globals.audio.play_sound(&globals.sfx.tile_break);
                    }

                    tile.set_state_index(TileState::BROKEN, Some(BROKEN_LIFETIME));
                }
            },
        );
        registry.push(cracked_state);

        // Broken
        debug_assert_eq!(registry.len(), TileState::BROKEN);
        let animator = Animator::load_new(assets, ResourcePaths::BATTLE_TILE_BROKEN_ANIMATION);
        let mut broken_state = TileState::new(String::from("Broken"), texture, animator);
        broken_state.hide_frame = true;
        broken_state.hide_body = true;
        broken_state.is_hole = true;
        broken_state.max_lifetime = Some(BROKEN_LIFETIME);

        broken_state.can_replace_callback =
            BattleCallback::new(|_, _, _, (_, _, state_index)| state_index != TileState::CRACKED);

        registry.push(broken_state);

        registry
    }

    pub fn complete_registry(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        resources: &SharedBattleResources,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let packages = &globals.tile_state_packages;

        for (info, namespace) in dependencies {
            if info.category != PackageCategory::TileState {
                continue;
            }

            let Some(package) = packages.package_or_fallback(*namespace, &info.id) else {
                continue;
            };

            let mut registry_iter = simulation.tile_states.iter();

            if registry_iter.any(|state| state.state_name == package.state_name) {
                continue;
            }

            let texture = assets.texture(game_io, &package.texture_path);
            let animator = Animator::load_new(assets, &package.animation_path);

            let mut tile_state = TileState::new(package.state_name.clone(), texture, animator);
            tile_state.max_lifetime = package.max_lifetime;
            tile_state.hide_frame = package.hide_frame;
            tile_state.hide_body = package.hide_body;
            tile_state.is_hole = package.hole;
            tile_state.cleanser_element = package.cleanser_element;

            let index = simulation.tile_states.len();
            simulation.tile_states.push(tile_state);

            let vm_manager = &resources.vm_manager;
            let Some(vm_index) = vm_manager.find_vm_from_info(info) else {
                continue;
            };

            let result =
                simulation.call_global(game_io, resources, vm_index, "tile_state_init", |lua| {
                    create_custom_tile_state_table(lua, index)
                });

            if let Err(err) = result {
                log::error!("{err}");
            }
        }
    }

    pub fn can_replace(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        resources: &SharedBattleResources,
        x: i32,
        y: i32,
        state_index: usize,
    ) -> bool {
        let Some(tile_state) = simulation.tile_states.get(state_index) else {
            return false;
        };

        let field = &mut simulation.field;
        let Some(tile) = field.tile_at_mut((x, y)) else {
            return false;
        };

        if tile_state.is_hole && !tile.reservations().is_empty() {
            // tile must be walkable for entities that are on or are moving to this tile
            return false;
        }

        let current_tile_state = simulation.tile_states.get(tile.state_index()).unwrap();
        let can_replace_callback = current_tile_state.can_replace_callback.clone();

        can_replace_callback.call(game_io, resources, simulation, (x, y, state_index))
    }
}
