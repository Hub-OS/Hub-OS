use super::Entity;
use crate::battle::{BattleCallback, BattleSimulation};
use crate::bindable::*;
use framework::prelude::GameIO;

#[derive(Default, Clone)]
pub struct Spell {
    pub requested_highlight: TileHighlight,
    pub hit_props: HitProperties,
    pub collision_callback: BattleCallback<EntityId>,
    pub attack_callback: BattleCallback<EntityId>,
}

impl Spell {
    pub fn create(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id = Entity::create(game_io, simulation);

        simulation
            .entities
            .insert(id.into(), (Spell::default(),))
            .unwrap();

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.ignore_hole_tiles = true;
        entity.ignore_negative_tile_effects = true;
        entity.can_move_to_callback = BattleCallback::stub(true);

        id
    }
}
