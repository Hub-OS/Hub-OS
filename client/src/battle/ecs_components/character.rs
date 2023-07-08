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
    pub rank: CharacterRank,
    pub cards: Vec<CardProperties>,
    pub namespace: PackageNamespace,
}

impl Character {
    fn new(rank: CharacterRank, namespace: PackageNamespace) -> Self {
        Self {
            rank,
            cards: Vec::new(),
            namespace,
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
        let character = simulation
            .entities
            .query_one_mut::<&mut Character>(entity_id.into())
            .unwrap();

        let namespace = character.namespace;
        let card_props = character.cards.pop().unwrap();

        let callback = BattleCallback::new(move |game_io, simulation, vms, _: ()| {
            let package_id = &card_props.package_id;

            let vm_index = match BattleSimulation::find_vm(vms, package_id, namespace) {
                Ok(vm_index) => vm_index,
                _ => {
                    log::error!("Failed to find vm for {package_id}");
                    return None;
                }
            };

            let lua = &vms[vm_index].lua;
            let card_init: rollback_mlua::Function = match lua.globals().get("card_init") {
                Ok(card_init) => card_init,
                _ => {
                    log::error!("{package_id} is missing card_init()");
                    return None;
                }
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

                let original_context_flags = {
                    // allow attacks to counter
                    let mut api_ctx = api_ctx.borrow_mut();
                    let entities = &mut api_ctx.simulation.entities;

                    let entity = entities
                        .query_one_mut::<&mut Entity>(entity_id.into())
                        .unwrap();

                    let original_flags = entity.hit_context.flags;
                    entity.hit_context.flags = HitFlag::NONE;

                    original_flags
                };

                // init card action
                let entity_table = create_entity_table(lua, entity_id)?;
                let lua_card_props = card_props.to_lua(lua)?;

                let table: rollback_mlua::Table = card_init.call((entity_table, lua_card_props))?;

                let index = table.raw_get("#id")?;
                id = Some(index);

                {
                    // revert context flags
                    let mut api_ctx = api_ctx.borrow_mut();
                    let entities = &mut api_ctx.simulation.entities;

                    let entity = entities
                        .query_one_mut::<&mut Entity>(entity_id.into())
                        .unwrap();
                    entity.hit_context.flags = original_context_flags;
                }

                Ok(())
            });

            if let Some(index) = id {
                if let Some(action) = simulation.actions.get_mut(index.into()) {
                    action.properties = card_props.clone();
                }
            }

            id
        });

        if let Some(index) = callback.call(game_io, simulation, vms, ()) {
            simulation.use_action(game_io, entity_id, index.into());
        }
    }
}
