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
    pub attack_boost: u8,
    pub speed_boost: u8,
    pub charge_boost: u8,
    pub card_view_size: u8,
    pub charging_time: FrameTime,
    pub max_charging_time: FrameTime,
    pub charge_sprite_index: TreeIndex,
    pub charge_animator: Animator,
    pub charged_color: Color,
    pub slide_when_moving: bool,
    pub forms: Vec<PlayerForm>,
    pub active_form: Option<usize>,
    pub modifiers: Arena<AbilityModifier>,
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
            attack_boost: 0,
            speed_boost: 0,
            charge_boost: 0,
            card_view_size: 5,
            charging_time: 0,
            max_charging_time: 100,
            charge_sprite_index,
            charge_animator: Animator::load_new(assets, ResourcePaths::BATTLE_CHARGE_ANIMATION),
            charged_color: Color::MAGENTA,
            slide_when_moving: false,
            forms: Vec::new(),
            active_form: None,
            modifiers: Arena::new(),
            normal_attack_callback: BattleCallback::stub(None),
            charged_attack_callback: BattleCallback::stub(None),
            special_attack_callback: BattleCallback::stub(None),
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
        let modifier_iter = self.modifiers.iter();
        let base_attack = modifier_iter
            .fold(1, |acc, (_, m)| acc + m.attack_boost as i32)
            .clamp(1, 5) as u8;

        base_attack + self.attack_boost
    }

    pub fn speed_level(&self) -> u8 {
        let modifier_iter = self.modifiers.iter();
        let base_speed = modifier_iter
            .fold(1, |acc, (_, m)| acc + m.speed_boost as i32)
            .clamp(1, 5) as u8;

        base_speed + self.speed_boost
    }

    pub fn charge_level(&self) -> u8 {
        let modifier_iter = self.modifiers.iter();
        let base_charge = modifier_iter
            .fold(1, |acc, (_, m)| acc + m.charge_boost as i32)
            .clamp(1, 5) as u8;

        base_charge + self.charge_boost
    }

    pub fn use_normal_attack(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        entity_id: EntityId,
    ) {
        let Ok(entity) = simulation.entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let modifier_iter = entity.modifiers.iter();
        let mut callbacks: Vec<_> = modifier_iter
            .flat_map(|(_, modifier)| modifier.normal_attack_callback.clone())
            .collect();

        callbacks.push(entity.normal_attack_callback.clone());

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
        let Ok(entity) = simulation.entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let modifier_iter = entity.modifiers.iter();
        let mut callbacks: Vec<_> = modifier_iter
            .flat_map(|(_, modifier)| modifier.charged_attack_callback.clone())
            .collect();

        callbacks.push(entity.charged_attack_callback.clone());

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
        let Ok(entity) = simulation.entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let modifier_iter = entity.modifiers.iter();
        let mut callbacks: Vec<_> = modifier_iter
            .flat_map(|(_, modifier)| modifier.special_attack_callback.clone())
            .collect();

        callbacks.push(entity.special_attack_callback.clone());

        for callback in callbacks {
            if let Some(index) = callback.call(game_io, simulation, vms, ()) {
                simulation.use_card_action(game_io, entity_id, index.into());
                return;
            }
        }
    }
}
