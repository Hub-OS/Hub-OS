use super::{Entity, Living, Spell};
use crate::battle::BattleSimulation;
use crate::bindable::EntityId;
use framework::prelude::GameIO;

#[derive(Default, Clone)]
pub struct Obstacle;

impl Obstacle {
    pub fn create(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id = Entity::create(game_io, simulation);

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        // obstacles should own their tile by default
        entity.share_tile = false;
        entity.auto_reserves_tiles = true;

        simulation
            .entities
            .insert(
                id.into(),
                (Spell::default(), Obstacle::default(), Living::default()),
            )
            .unwrap();

        id
    }
}
