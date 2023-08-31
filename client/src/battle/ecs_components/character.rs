use crate::battle::{
    Action, BattleCallback, BattleSimulation, Entity, SharedBattleResources, TileState,
};
use crate::bindable::*;
use crate::lua_api::create_entity_table;
use crate::packages::PackageNamespace;
use framework::prelude::{GameIO, Vec2};
use packets::structures::PackageId;

use super::{Artifact, Living};

#[derive(Clone)]
pub struct Character {
    pub namespace: PackageNamespace,
    pub rank: CharacterRank,
    pub cards: Vec<CardProperties>, // stores cards reversed
    pub card_use_requested: bool,
    pub next_card_mutation: Option<usize>, // stores card index, invert to get a usable index
}

impl Character {
    fn new(rank: CharacterRank, namespace: PackageNamespace) -> Self {
        Self {
            namespace,
            rank,
            cards: Vec::new(),
            card_use_requested: false,
            next_card_mutation: None,
        }
    }

    pub fn create(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        rank: CharacterRank,
        namespace: PackageNamespace,
    ) -> rollback_mlua::Result<EntityId> {
        let id = Entity::create(game_io, simulation);

        simulation
            .entities
            .insert(
                id.into(),
                (Character::new(rank, namespace), Living::default()),
            )
            .unwrap();

        let (entity, living) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Living)>(id.into())
            .unwrap();

        // characters should own their tiles by default
        entity.share_tile = false;
        entity.auto_reserves_tiles = true;

        let elemental_weakness_aux_prop = AuxProp::new()
            .with_requirement(AuxRequirement::HitDamage(Comparison::GT, 0))
            .with_requirement(AuxRequirement::HitElementIsWeakness)
            .with_callback(BattleCallback::new(move |game_io, _, simulation, _| {
                let entities = &mut simulation.entities;
                let entity = entities.query_one_mut::<&Entity>(id.into()).unwrap();

                // spawn alert artifact
                let mut alert_position = entity.full_position();
                alert_position.offset += Vec2::new(0.0, -entity.height);

                let alert_id = Artifact::create_alert(game_io, simulation);
                let alert_entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(alert_id.into())
                    .unwrap();

                alert_entity.copy_full_position(alert_position);
                alert_entity.pending_spawn = true;
            }));

        living.add_aux_prop(elemental_weakness_aux_prop);

        entity.can_move_to_callback = BattleCallback::new(move |_, _, simulation, dest| {
            let tile = match simulation.field.tile_at_mut(dest) {
                Some(tile) => tile,
                None => return false,
            };

            if tile.state_index() == TileState::HIDDEN {
                // can't walk on hidden tiles, even with ignore_hole_tiles
                return false;
            }

            let entity = simulation
                .entities
                .query_one_mut::<&Entity>(id.into())
                .unwrap();

            if !entity.ignore_hole_tiles && simulation.tile_states[tile.state_index()].is_hole {
                // can't walk on holes
                return false;
            }

            if tile.team() != entity.team && tile.team() != Team::Other {
                // tile can't belong to the opponent team
                return false;
            }

            if entity.share_tile {
                return true;
            }

            for entity_id in tile.reservations().iter().cloned() {
                if entity_id == id {
                    continue;
                }

                let entities = &mut simulation.entities;
                let Ok(other_entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                    // non entity or erased is reserving?
                    continue;
                };

                if !other_entity.share_tile {
                    // another entity is reserving this tile and refusing to share
                    return false;
                }
            }

            true
        });

        entity.delete_callback = BattleCallback::new(move |_, _, simulation, _| {
            crate::battle::delete_character_animation(simulation, id, None);
        });

        Ok(id)
    }

    pub fn load(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        package_id: &PackageId,
        namespace: PackageNamespace,
        rank: CharacterRank,
    ) -> rollback_mlua::Result<EntityId> {
        let id = Self::create(game_io, simulation, rank, namespace)?;

        let vm_index = resources.vm_manager.find_vm(package_id, namespace)?;
        simulation.call_global(game_io, resources, vm_index, "character_init", move |lua| {
            create_entity_table(lua, id)
        })?;

        Ok(id)
    }

    pub fn use_card(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        let (entity, character) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Character)>(entity_id.into())
            .unwrap();

        // allow attacks to counter
        let original_context_flags = entity.hit_context.flags;
        entity.hit_context.flags = HitFlag::NONE;

        // create card action
        let namespace = character.namespace;
        let card_props = character.cards.pop().unwrap();
        let action_index = Action::create_from_card_properties(
            game_io,
            resources,
            simulation,
            entity_id,
            namespace,
            &card_props,
        );

        // use the action or spawn a poof
        if let Some(index) = action_index {
            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(entity_id.into())
                .unwrap();

            entity.action_queue.push_back(index);
        } else {
            Artifact::create_card_poof(game_io, simulation, entity_id);
        }

        // revert context flags
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .unwrap();
        entity.hit_context.flags = original_context_flags;
    }

    pub fn mutate_cards(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        loop {
            let entities = &mut simulation.entities;
            let mut character_iter = entities.query_mut::<&mut Character>().into_iter();

            let Some((id, character)) =
                character_iter.find(|(_, character)| character.next_card_mutation.is_some())
            else {
                break;
            };

            // get next card index
            let lua_card_index = character.next_card_mutation.unwrap();

            // update the card index
            character.next_card_mutation = Some(lua_card_index + 1);

            let card_index = character.invert_card_index(lua_card_index);

            // get the card or mark the mutate state as complete
            let Some(card) = &character.cards.get(card_index) else {
                character.next_card_mutation = None;
                continue;
            };

            // make sure the vm + callback exists before calling
            // avoids an error message from simulation.call_global()
            // function card_mutate is optional to implement
            let vm_manager = &resources.vm_manager;
            let Ok(vm_index) = vm_manager.find_vm(&card.package_id, character.namespace) else {
                continue;
            };

            let lua = &vm_manager.vms()[vm_index].lua;
            let callback_exists = lua.globals().contains_key("card_mutate").unwrap_or(false);

            if !callback_exists {
                continue;
            }

            let _ = simulation.call_global(game_io, resources, vm_index, "card_mutate", |lua| {
                Ok((create_entity_table(lua, id.into())?, lua_card_index + 1))
            });
        }
    }

    pub fn invert_card_index(&self, index: usize) -> usize {
        self.cards.len().wrapping_sub(index).wrapping_sub(1)
    }
}
