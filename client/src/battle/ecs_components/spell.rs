use super::Entity;
use crate::battle::{AttackBox, BattleCallback, BattleSimulation};
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

    pub fn attack_tile(simulation: &mut BattleSimulation, id: EntityId, x: i32, y: i32) {
        let entities = &mut simulation.entities;
        let Ok((entity, spell)) = entities.query_one_mut::<(&Entity, &Spell)>(id.into()) else {
            return;
        };

        let queued_attacks = &mut simulation.queued_attacks;
        let is_same_attack =
            |attack: &AttackBox| attack.attacker_id == entity.id && attack.x == x && attack.y == y;

        if !queued_attacks.iter().any(is_same_attack) {
            let attack_box = AttackBox::new_from((x, y), entity, spell);
            queued_attacks.push(attack_box);
        }
    }
}
