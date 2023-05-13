use crate::battle::{BattleCallback, BattleScriptContext, BattleSimulation, Entity, RollbackVM};
use crate::bindable::*;
use crate::lua_api::create_entity_table;
use crate::packages::PackageNamespace;
use crate::resources::Globals;
use framework::prelude::GameIO;
use std::cell::RefCell;

#[derive(Clone)]
pub struct Character {
    pub rank: CharacterRank,
    pub cards: Vec<CardProperties>,
    pub namespace: PackageNamespace,
}

impl Character {
    pub fn new(rank: CharacterRank, namespace: PackageNamespace) -> Self {
        Self {
            rank,
            cards: Vec::new(),
            namespace,
        }
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
