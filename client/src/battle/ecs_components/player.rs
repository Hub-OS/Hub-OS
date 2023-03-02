use crate::battle::*;
use crate::bindable::*;
use crate::packages::PackageNamespace;
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use framework::prelude::*;
use generational_arena::Arena;

#[derive(Clone)]
pub struct Player {
    pub index: usize,
    pub local: bool,
    pub cards: Vec<Card>,
    pub card_use_requested: bool,
    pub can_flip: bool,
    pub flip_requested: bool,
    pub attack_boost: u8,
    pub rapid_boost: u8,
    pub charge_boost: u8,
    pub card_view_size: u8,
    pub charging_time: FrameTime,
    pub charge_sprite_index: TreeIndex,
    pub charge_animator: Animator,
    pub charged_color: Color,
    pub slide_when_moving: bool,
    pub forms: Vec<PlayerForm>,
    pub active_form: Option<usize>,
    pub augments: Arena<Augment>,
    pub calculate_charge_time_callback: BattleCallback<u8, FrameTime>,
    pub normal_attack_callback: BattleCallback<(), Option<GenerationalIndex>>,
    pub charged_attack_callback: BattleCallback<(), Option<GenerationalIndex>>,
    pub special_attack_callback: BattleCallback<(), Option<GenerationalIndex>>,
}

impl Player {
    pub const IDLE_STATE: &str = "PLAYER_IDLE";
    pub const CHARGE_DELAY: FrameTime = 10;

    pub const MOVE_FRAMES: [DerivedFrame; 7] = [
        DerivedFrame::new(0, 1),
        DerivedFrame::new(1, 1),
        DerivedFrame::new(2, 1),
        DerivedFrame::new(3, 1),
        DerivedFrame::new(2, 1),
        DerivedFrame::new(1, 1),
        DerivedFrame::new(0, 1),
    ];

    pub const HIT_FRAMES: [DerivedFrame; 2] = [DerivedFrame::new(0, 15), DerivedFrame::new(1, 7)];

    pub fn new(
        game_io: &GameIO,
        index: usize,
        local: bool,
        charge_sprite_index: TreeIndex,
        cards: Vec<Card>,
    ) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;

        Self {
            index,
            local,
            cards,
            card_use_requested: false,
            can_flip: true,
            flip_requested: false,
            attack_boost: 0,
            rapid_boost: 0,
            charge_boost: 0,
            card_view_size: 5,
            charging_time: 0,
            charge_sprite_index,
            charge_animator: Animator::load_new(assets, ResourcePaths::BATTLE_CHARGE_ANIMATION),
            charged_color: Color::MAGENTA,
            slide_when_moving: false,
            forms: Vec::new(),
            active_form: None,
            augments: Arena::new(),
            calculate_charge_time_callback: BattleCallback::new(|_, _, _, level| {
                Self::calculate_default_charge_time(level)
            }),
            normal_attack_callback: BattleCallback::stub(None),
            charged_attack_callback: BattleCallback::stub(None),
            special_attack_callback: BattleCallback::stub(None),
        }
    }

    pub fn initialize_uninitialized(simulation: &mut BattleSimulation) {
        // resolve flippable defaults by team (Team::Other will always be true)
        let mut default_red_flippable = false;
        let mut default_blue_flippable = false;

        let field_col_count = simulation.field.cols();

        for ((col, _), tile) in simulation.field.iter_mut() {
            if col == 1 && tile.original_team() != Team::Red {
                default_red_flippable = true;
            }

            if col + 2 == field_col_count && tile.original_team() != Team::Blue {
                default_blue_flippable = true;
            }
        }

        // initialize uninitalized
        let config = &simulation.config;

        type PlayerQuery<'a> = (&'a mut Entity, &'a mut Player, &'a Living);

        for (_, (entity, player, living)) in simulation.entities.query_mut::<PlayerQuery>() {
            // track the local player's health
            if player.local {
                simulation.local_player_id = entity.id;
                simulation.local_health_ui.snap_health(living.health);
            }

            // initialize position
            let pos = config
                .player_spawn_positions
                .get(player.index)
                .cloned()
                .unwrap_or_default();

            entity.x = pos.0;
            entity.y = pos.1;

            // initalize team
            let tile = simulation.field.tile_at_mut((entity.x, entity.y));
            entity.team = tile.map(|tile| tile.team()).unwrap_or_default();

            // initalize flippable
            let flippable_config = &config.player_flippable;
            let can_flip_setting = flippable_config.get(player.index).cloned().flatten();

            player.can_flip = can_flip_setting.unwrap_or(match entity.team {
                Team::Red => default_red_flippable,
                Team::Blue => default_blue_flippable,
                _ => true,
            });

            // initialize animation state
            let animator = &mut simulation.animators[entity.animator_index];

            if animator.current_state().is_none() {
                let callbacks = animator.set_state(Player::IDLE_STATE);
                animator.set_loop_mode(AnimatorLoopMode::Loop);
                simulation.pending_callbacks.extend(callbacks);
            }
        }
    }

    pub fn available_forms(&self) -> impl Iterator<Item = (usize, &PlayerForm)> {
        self.forms
            .iter()
            .enumerate()
            .filter(|(_, form)| !form.activated)
    }

    pub fn namespace(&self) -> PackageNamespace {
        if self.local {
            PackageNamespace::Local
        } else {
            PackageNamespace::Remote(self.index)
        }
    }

    pub fn attack_level(&self) -> u8 {
        let augment_iter = self.augments.iter();
        let base_attack = augment_iter
            .fold(1, |acc, (_, m)| {
                acc + m.attack_boost as i32 * m.level as i32
            })
            .clamp(1, 5) as u8;

        base_attack + self.attack_boost
    }

    pub fn rapid_level(&self) -> u8 {
        let augment_iter = self.augments.iter();
        let base_speed = augment_iter
            .fold(1, |acc, (_, m)| acc + m.rapid_boost as i32 * m.level as i32)
            .clamp(1, 5) as u8;

        base_speed + self.rapid_boost
    }

    pub fn charge_level(&self) -> u8 {
        let augment_iter = self.augments.iter();
        let base_charge = augment_iter
            .fold(1, |acc, (_, m)| {
                acc + m.charge_boost as i32 * m.level as i32
            })
            .clamp(1, 5) as u8;

        base_charge + self.charge_boost
    }

    pub fn calculate_default_charge_time(level: u8) -> FrameTime {
        match level {
            0 | 1 => 60,
            2 => 70,
            3 => 80,
            4 => 90,
            _ => 100,
        }
    }

    pub fn calculate_charge_time(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        entity_id: EntityId,
        level: Option<u8>,
    ) -> FrameTime {
        let Ok(player) = simulation.entities.query_one_mut::<&Player>(entity_id.into()) else {
            return 0;
        };

        let level = level.unwrap_or_else(|| player.charge_level());

        let augment_iter = player.augments.iter();
        let augment_callback = augment_iter
            .flat_map(|(_, augment)| augment.calculate_charge_time_callback.clone())
            .next();

        let callback = augment_callback
            .or_else(|| {
                player.active_form.and_then(|index| {
                    let form = player.forms.get(index)?;
                    form.calculate_charge_time_callback.clone()
                })
            })
            .unwrap_or_else(|| player.calculate_charge_time_callback.clone());

        callback.call(game_io, simulation, vms, level)
    }

    pub fn use_normal_attack(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        entity_id: EntityId,
    ) {
        let Ok(player) = simulation.entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let augment_iter = player.augments.iter();
        let mut callbacks: Vec<_> = augment_iter
            .flat_map(|(_, augment)| augment.normal_attack_callback.clone())
            .collect();

        callbacks.push(player.normal_attack_callback.clone());

        for callback in callbacks {
            if let Some(index) = callback.call(game_io, simulation, vms, ()) {
                simulation.use_card_action(game_io, entity_id, index.into());
                return;
            }
        }
    }

    pub fn use_charged_attack(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        entity_id: EntityId,
    ) {
        let Ok(player) = simulation.entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        // augment
        let augment_iter = player.augments.iter();
        let mut callbacks: Vec<_> = augment_iter
            .flat_map(|(_, augment)| augment.charged_attack_callback.clone())
            .collect();

        // form
        let form_callback = player
            .active_form
            .and_then(|index| player.forms.get(index))
            .and_then(|form| form.special_attack_callback.clone());

        if let Some(callback) = form_callback {
            callbacks.push(callback);
        }

        // base
        callbacks.push(player.charged_attack_callback.clone());

        for callback in callbacks {
            if let Some(index) = callback.call(game_io, simulation, vms, ()) {
                simulation.use_card_action(game_io, entity_id, index.into());
                return;
            }
        }
    }

    pub fn use_special_attack(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        entity_id: EntityId,
    ) {
        let Ok(player) = simulation.entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        // augment
        let augment_iter = player.augments.iter();
        let mut callbacks: Vec<_> = augment_iter
            .flat_map(|(_, augment)| augment.special_attack_callback.clone())
            .collect();

        // form
        let form_callback = player
            .active_form
            .and_then(|index| player.forms.get(index))
            .and_then(|form| form.special_attack_callback.clone());

        if let Some(callback) = form_callback {
            callbacks.push(callback);
        }

        // base
        callbacks.push(player.special_attack_callback.clone());

        for callback in callbacks {
            if let Some(index) = callback.call(game_io, simulation, vms, ()) {
                simulation.use_card_action(game_io, entity_id, index.into());
                return;
            }
        }
    }
}
