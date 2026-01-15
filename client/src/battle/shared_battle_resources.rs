use super::external_events::ExternalEvents;
use super::{BattleConfig, BattleMeta, BattleSimulation, StatusRegistry, TileState};
use crate::bindable::{AuxVariable, MathExpr};
use crate::lua_api::BattleVmManager;
use crate::packages::{PackageInfo, PackageNamespace};
use crate::render::ui::GlyphAtlas;
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::scenes::BattleEvent;
use crate::{CardRecipes, RESOLUTION_F};
use framework::math::Rect;
use framework::prelude::{Color, GameIO, Sprite};
use packets::structures::PackageCategory;
use std::borrow::Cow;
use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::sync::Arc;

/// Resources that are shared between battle snapshots
pub struct SharedBattleResources {
    pub config: RefCell<BattleConfig>,
    pub vm_manager: BattleVmManager,
    pub status_registry: StatusRegistry,
    pub recipe_animator: Animator,
    pub recipes: CardRecipes,
    pub alert_animator: RefCell<Animator>,
    pub math_expressions:
        RefCell<HashMap<String, rollback_mlua::Result<MathExpr<f32, AuxVariable>>>>,
    pub glyph_atlases: RefCell<HashMap<(Cow<'static, str>, Cow<'static, str>), Arc<GlyphAtlas>>>,
    pub battle_fade_color: Cell<Color>,
    pub ui_fade_color: Cell<Color>,
    pub fade_sprite: RefCell<Sprite>,
    pub music_stack_depth: usize,
    pub event_sender: flume::Sender<BattleEvent>,
    pub event_receiver: flume::Receiver<BattleEvent>,
    pub external_events: ExternalEvents,
}

impl SharedBattleResources {
    pub fn new(game_io: &GameIO, meta: &BattleMeta) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let (event_sender, event_receiver) = flume::unbounded();

        let fade_sprite_texture = assets.texture(game_io, ResourcePaths::PIXEL);

        // large fade sprite to account for camera differences between ui and battle
        let mut fade_sprite = Sprite::new(game_io, fade_sprite_texture);
        fade_sprite.set_bounds(Rect::from_corners(-RESOLUTION_F, RESOLUTION_F));

        Self {
            config: RefCell::new(BattleConfig::new(globals, meta.player_count)),
            vm_manager: BattleVmManager::new(),
            status_registry: StatusRegistry::new(),
            recipe_animator: Animator::load_new(assets, ResourcePaths::BATTLE_RECIPE_ANIMATION)
                .with_state("DEFAULT"),
            recipes: CardRecipes::default(),
            alert_animator: RefCell::new(
                Animator::load_new(assets, ResourcePaths::BATTLE_ALERT_ANIMATION).with_state("UI"),
            ),
            math_expressions: Default::default(),
            glyph_atlases: Default::default(),
            battle_fade_color: Default::default(),
            ui_fade_color: Default::default(),
            fade_sprite: fade_sprite.into(),
            music_stack_depth: globals.audio.music_stack_len() + 1,
            event_sender,
            event_receiver,
            external_events: ExternalEvents::new(),
        }
    }

    pub fn load_encounter_vms(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        meta: &BattleMeta,
    ) {
        let mut dependencies = meta.encounter_dependencies(game_io);

        // sort by namespace, ensuring proper load order
        dependencies.sort_by_key(|(_, ns)| *ns);

        BattleVmManager::init_internal_vm(game_io, self, simulation);
        self.init_dependencies(game_io, simulation, &dependencies);

        if let Some((ns, package_id)) = &meta.encounter_package_pair {
            self.vm_manager.encounter_vm = self.vm_manager.find_vm(package_id, *ns).ok();
        }
    }

    pub fn load_player_vms(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        meta: &BattleMeta,
    ) {
        let mut dependencies = meta.player_dependencies(game_io);

        // sort by namespace, ensuring proper load order
        dependencies.sort_by_key(|(_, ns)| *ns);

        let globals = Globals::from_resources(game_io);

        // load recipes
        for setup in &meta.player_setups {
            let ns = setup.namespace();

            for output in &setup.recipes {
                if let Some(package) = globals.card_packages.package_or_fallback(ns, output) {
                    self.recipes.load_from_package(ns, package)
                }
            }
        }

        self.init_dependencies(game_io, simulation, &dependencies);
    }

    fn init_dependencies(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) {
        // load tile states + statuses first
        BattleVmManager::init_dependency_vms(
            game_io,
            self,
            simulation,
            dependencies.iter().filter(|(p, _)| {
                matches!(
                    p.category,
                    PackageCategory::TileState | PackageCategory::Status
                )
            }),
        );

        // register tile states
        TileState::complete_registry(game_io, simulation, self, dependencies);

        // register statuses
        self.status_registry
            .init(game_io, &self.vm_manager, dependencies);

        // load remaining packages
        BattleVmManager::init_dependency_vms(
            game_io,
            self,
            simulation,
            dependencies.iter().filter(|(p, _)| {
                !matches!(
                    p.category,
                    PackageCategory::TileState | PackageCategory::Status
                )
            }),
        );
    }

    pub fn parse_math_expr(
        &self,
        source: String,
    ) -> rollback_mlua::Result<MathExpr<f32, AuxVariable>> {
        let mut expressions = self.math_expressions.borrow_mut();

        expressions
            .entry(source)
            .or_insert_with_key(|source| {
                MathExpr::parse(source)
                    .map_err(|err| rollback_mlua::Error::RuntimeError(err.to_string()))
            })
            .clone()
    }

    pub fn draw_fade_sprite(&self, sprite_queue: &mut SpriteColorQueue, color: Color) {
        if color != Color::TRANSPARENT {
            let fade_sprite = &mut *self.fade_sprite.borrow_mut();
            fade_sprite.set_color(color);
            sprite_queue.draw_sprite(fade_sprite);
        }
    }
}
