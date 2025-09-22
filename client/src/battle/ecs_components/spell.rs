use super::Entity;
use crate::battle::{AttackBox, BattleCallback, BattleSimulation, CanMoveToCallback};
use crate::bindable::*;
use framework::prelude::GameIO;

#[derive(Default, Clone)]
pub struct Spell {
    pub requested_highlight: TileHighlight,
    pub hit_props: HitProperties,
}

impl Spell {
    pub fn create(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id = Entity::create(game_io, simulation);

        let components = (
            Spell::default(),
            CanMoveToCallback(BattleCallback::stub(true)),
        );
        simulation.entities.insert(id.into(), components).unwrap();

        let entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.ignore_hole_tiles = true;
        entity.ignore_negative_tile_effects = true;

        id
    }

    pub fn attack_tile(simulation: &mut BattleSimulation, id: EntityId, x: i32, y: i32) {
        let entities = &mut simulation.entities;
        let Ok((entity, spell)) = entities.query_one_mut::<(&Entity, &Spell)>(id.into()) else {
            return;
        };

        let queued_attacks = &mut simulation.queued_attacks;
        let is_same_attack = |attack: &&mut AttackBox| -> bool {
            attack.attacker_id == id && attack.x == x && attack.y == y
        };

        if let Some(attack_box) = queued_attacks.iter_mut().find(is_same_attack) {
            attack_box.refresh();
        } else {
            let attack_box = AttackBox::new_from((x, y), id, entity, spell);
            queued_attacks.push(attack_box);
        }
    }
}
