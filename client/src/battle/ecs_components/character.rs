use crate::battle::{
    BattleCallback, BattleScriptContext, BattleSimulation, Entity, RollbackVM, TileState,
};
use crate::bindable::*;
use crate::lua_api::create_entity_table;
use crate::packages::PackageNamespace;
use crate::resources::Globals;
use framework::prelude::{GameIO, Vec2};
use packets::structures::PackageId;
use std::cell::RefCell;

use super::{Artifact, Living};

#[derive(Clone)]
pub struct Character {
    pub namespace: PackageNamespace,
    pub rank: CharacterRank,
    pub cards: Vec<CardProperties>,        // stores cards reversed
    pub next_card_mutation: Option<usize>, // stores card index, invert to get a usable index
}

impl Character {
    fn new(rank: CharacterRank, namespace: PackageNamespace) -> Self {
        Self {
            namespace,
            rank,
            cards: Vec::new(),
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

        // hit callback for alert symbol
        living.register_hit_callback(BattleCallback::new(
            move |game_io, simulation, _, hit_props: HitProperties| {
                if hit_props.damage == 0 {
                    return;
                }

                let entity = simulation
                    .entities
                    .query_one_mut::<&Entity>(id.into())
                    .unwrap();

                if !entity.element.is_weak_to(hit_props.element)
                    && !entity.element.is_weak_to(hit_props.secondary_element)
                {
                    // not super effective
                    return;
                }

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
            },
        ));

        entity.can_move_to_callback = BattleCallback::new(move |_, simulation, _, dest| {
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

        entity.delete_callback = BattleCallback::new(move |_, simulation, _, _| {
            crate::battle::delete_character_animation(simulation, id, None);
        });

        Ok(id)
    }

    pub fn load(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        package_id: &PackageId,
        namespace: PackageNamespace,
        rank: CharacterRank,
    ) -> rollback_mlua::Result<EntityId> {
        let id = Self::create(game_io, simulation, rank, namespace)?;

        let vm_index = BattleSimulation::find_vm(vms, package_id, namespace)?;
        simulation.call_global(game_io, vms, vm_index, "character_init", move |lua| {
            create_entity_table(lua, id)
        })?;

        Ok(id)
    }

    pub fn use_card(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        entity_id: EntityId,
    ) {
        let (entity, character) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Character)>(entity_id.into())
            .unwrap();

        // allow attacks to counter
        let original_context_flags = entity.hit_context.flags;
        entity.hit_context.flags = HitFlag::NONE;

        // call "card_init"
        let namespace = character.namespace;
        let card_props = character.cards.pop().unwrap();

        let callback = BattleCallback::new(move |game_io, simulation, vms, _: ()| {
            let package_id = &card_props.package_id;

            if package_id.is_blank() {
                return None;
            }

            let Ok(vm_index) = BattleSimulation::find_vm(vms, package_id, namespace) else {
                log::error!("Failed to find vm for {package_id}");
                return None;
            };

            let lua = &vms[vm_index].lua;
            let Ok(card_init) = lua.globals().get::<_, rollback_mlua::Function>("card_init") else {
                log::error!("{package_id} is missing card_init()");
                return None;
            };

            let api_ctx = RefCell::new(BattleScriptContext {
                vm_index,
                vms,
                game_io,
                simulation,
            });

            let lua_api = &game_io.resource::<Globals>().unwrap().battle_api;
            let mut id: Option<GenerationalIndex> = None;

            lua_api.inject_dynamic(lua, &api_ctx, |lua| {
                use rollback_mlua::ToLua;

                // init card action
                let entity_table = create_entity_table(lua, entity_id)?;
                let lua_card_props = card_props.to_lua(lua)?;

                let optional_table = match card_init
                    .call::<_, Option<rollback_mlua::Table>>((entity_table, lua_card_props))
                {
                    Ok(optional_table) => optional_table,
                    Err(err) => {
                        log::error!("{package_id}: {err}");
                        return Ok(());
                    }
                };

                id = optional_table
                    .map(|table| table.raw_get("#id"))
                    .transpose()?;

                Ok(())
            });

            // set card properties on the card action
            if let Some(index) = id {
                if let Some(action) = simulation.actions.get_mut(index.into()) {
                    action.properties = card_props.clone();
                }
            }

            id
        });

        // use the action or spawn a poof
        if let Some(index) = callback.call(game_io, simulation, vms, ()) {
            simulation.use_action(game_io, entity_id, index.into());
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

    pub fn mutate_cards(game_io: &GameIO, simulation: &mut BattleSimulation, vms: &[RollbackVM]) {
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
            let Ok(vm_index) =
                BattleSimulation::find_vm(vms, &card.package_id, character.namespace)
            else {
                continue;
            };

            let lua = &vms[vm_index].lua;
            let callback_exists = lua.globals().contains_key("card_mutate").unwrap_or(false);

            if !callback_exists {
                continue;
            }

            let _ = simulation.call_global(game_io, vms, vm_index, "card_mutate", |lua| {
                Ok((create_entity_table(lua, id.into())?, lua_card_index))
            });
        }
    }

    pub fn invert_card_index(&self, index: usize) -> usize {
        self.cards.len().wrapping_sub(index).wrapping_sub(1)
    }
}
