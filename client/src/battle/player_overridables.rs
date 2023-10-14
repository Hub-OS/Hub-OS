use super::{BattleCallback, BattleSimulation, Entity, Player, SharedBattleResources};
use crate::bindable::{CardProperties, EntityId, GenerationalIndex};
use crate::render::FrameTime;
use framework::prelude::GameIO;
use packets::structures::Direction;

#[derive(Default, Clone)]
pub struct PlayerOverridables {
    pub calculate_charge_time: Option<BattleCallback<u8, FrameTime>>,
    pub normal_attack: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub charged_attack: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub special_attack: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub can_charge_card: Option<BattleCallback<CardProperties, bool>>,
    pub charged_card: Option<BattleCallback<CardProperties, Option<GenerationalIndex>>>,
    pub movement: Option<BattleCallback<Direction>>,
}

impl PlayerOverridables {
    pub fn default_calculate_charge_time(level: u8) -> FrameTime {
        match level {
            0 | 1 => 100,
            2 => 90,
            3 => 80,
            4 => 70,
            _ => 60,
        }
    }

    pub fn default_movement(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        direction: Direction,
    ) {
        let entities = &mut simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
            return;
        };

        let mut dest = (entity.x, entity.y);
        let offset = direction.i32_vector();

        if offset.1 != 0 {
            dest.1 += offset.1;
        } else {
            dest.0 += offset.0;
        }

        let can_move_to_callback = entity.can_move_to_callback.clone();

        if can_move_to_callback.call(game_io, resources, simulation, dest) {
            Player::queue_default_movement(simulation, entity_id, dest);
        }
    }

    pub fn flat_map_for<T: 'static>(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        map: impl Fn(&Self) -> Option<T> + Clone + 'static,
    ) -> impl Iterator<Item = T> + '_ {
        let entities = &mut simulation.entities;
        let player_iter = entities
            .query_one_mut::<&Player>(entity_id.into())
            .into_iter();

        player_iter.flat_map(move |player| {
            // form
            let form_iter = player
                .active_form
                .into_iter()
                .flat_map(|index| player.forms.get(index));

            let form_map = map.clone();
            let form_callback_iter = form_iter.flat_map(move |form| form_map(&form.overridables));

            // augment
            let augment_iter = player.augments.values();
            let augment_map = map.clone();
            let augment_callback_iter =
                augment_iter.flat_map(move |augment| augment_map(&augment.overridables));

            // base
            let player_callback_iter = map(&player.overridables).into_iter();

            form_callback_iter
                .chain(augment_callback_iter)
                .chain(player_callback_iter)
        })
    }
}
