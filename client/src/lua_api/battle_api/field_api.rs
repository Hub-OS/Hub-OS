use framework::prelude::IVec2;

use super::errors::entity_not_found;
use super::tile_api::create_tile_table;
use super::{create_entity_table, BattleLuaApi, FIELD_TABLE};
use crate::battle::{
    BattleCallback, BattleScriptContext, Character, Entity, Living, Obstacle, Player, Spell,
};
use crate::bindable::EntityID;
use crate::resources::Globals;
use std::cell::RefCell;

pub fn inject_field_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(FIELD_TABLE, "tile_at", |api_ctx, lua, params| {
        let (_, x, y): (rollback_mlua::Table, i32, i32) = lua.unpack_multi(params)?;

        let api_ctx = api_ctx.borrow();

        if !api_ctx.simulation.field.in_bounds((x, y)) {
            return lua.pack_multi(());
        }

        lua.pack_multi(create_tile_table(lua, (x, y))?)
    });

    lua_api.add_dynamic_function(FIELD_TABLE, "width", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();

        lua.pack_multi(api_ctx.simulation.field.cols())
    });

    lua_api.add_dynamic_function(FIELD_TABLE, "height", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();

        lua.pack_multi(api_ctx.simulation.field.rows())
    });

    lua_api.add_dynamic_function(FIELD_TABLE, "spawn", |api_ctx, lua, params| {
        let (_, entity_table, rest): (
            rollback_mlua::Table,
            rollback_mlua::Table,
            rollback_mlua::MultiValue,
        ) = lua.unpack_multi(params)?;

        let (x, y): (i32, i32) = lua.unpack_multi(rest.clone()).or_else(
            move |_| -> rollback_mlua::Result<(i32, i32)> {
                let tile_table: rollback_mlua::Table = lua.unpack_multi(rest)?;

                let x = tile_table.raw_get("#x")?;
                let y = tile_table.raw_get("#y")?;

                Ok((x, y))
            },
        )?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        if !api_ctx.simulation.field.in_bounds((x, y)) {
            return lua.pack_multi(());
        }

        let id: EntityID = entity_table.raw_get("#id")?;

        let entities = &mut api_ctx.simulation.entities;
        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        if entity.spawned {
            return lua.pack_multi(());
        }

        entity.x = x;
        entity.y = y;
        entity.pending_spawn = true;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(FIELD_TABLE, "get_entity", |api_ctx, lua, params| {
        let (_, id): (rollback_mlua::Table, EntityID) = lua.unpack_multi(params)?;

        let api_ctx = api_ctx.borrow();

        if !api_ctx.simulation.entities.contains(id.into()) {
            return lua.pack_multi(());
        }

        lua.pack_multi(create_entity_table(lua, id)?)
    });

    generate_find_hittable_fn::<()>(lua_api, "find_entities");
    generate_find_hittable_fn::<&Character>(lua_api, "find_characters");
    generate_find_hittable_fn::<&Obstacle>(lua_api, "find_obstacles");
    generate_find_hittable_fn::<&Player>(lua_api, "find_players");
    generate_find_entity_fn::<hecs::Without<&Spell, &Obstacle>>(lua_api, "find_spells");

    generate_find_nearest_hittable_fn::<&Character>(lua_api, "find_nearest_characters");
    generate_find_nearest_hittable_fn::<&Player>(lua_api, "find_nearest_players");

    lua_api.add_dynamic_function(FIELD_TABLE, "find_tiles", |api_ctx, lua, params| {
        let (_, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let tables = {
            // scope to prevent RefCell borrow escaping when control is passed back to lua
            let api_ctx = api_ctx.borrow();
            let field = &api_ctx.simulation.field;

            let cols = field.cols();
            let rows = field.rows();

            let mut tables: Vec<rollback_mlua::Table> = Vec::with_capacity(cols * rows);

            for i in 0..cols as i32 {
                for j in 0..rows as i32 {
                    tables.push(create_tile_table(lua, (i, j))?);
                }
            }

            tables
        };

        let filtered_tables: Vec<rollback_mlua::Table> = tables
            .into_iter()
            .filter(|table| {
                callback.call::<_, bool>(table.clone()).unwrap_or_else(|e| {
                    log::error!("{e}");
                    false
                })
            })
            .collect();

        lua.pack_multi(filtered_tables)
    });

    // todo: should delete listeners move to entity api?
    lua_api.add_dynamic_function(FIELD_TABLE, "notify_on_delete", |api_ctx, lua, params| {
        let (_, target_id, observer_id, callback): (
            rollback_mlua::Table,
            EntityID,
            EntityID,
            rollback_mlua::Function,
        ) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let entities = &mut api_ctx.simulation.entities;
        let entity = entities
            .query_one_mut::<&mut Entity>(target_id.into())
            .map_err(|_| entity_not_found())?;

        let vm_index = api_ctx.vm_index;

        let callback_key = lua.create_registry_value(callback)?;
        let callback = BattleCallback::new(move |game_io, simulation, vms, _: ()| {
            let lua_api = &game_io.resource::<Globals>().unwrap().battle_api;

            let lua = &vms[vm_index].lua;

            let api_ctx = RefCell::new(BattleScriptContext {
                vm_index,
                game_io,
                simulation,
                vms,
            });

            lua_api.inject_dynamic(lua, &api_ctx, |lua| {
                let callback: rollback_mlua::Function = lua.registry_value(&callback_key)?;

                let observer_exists = {
                    let simulation = &api_ctx.borrow().simulation;
                    simulation.entities.contains(observer_id.into())
                };

                if !observer_exists {
                    return Ok(());
                }

                let target_table = create_entity_table(lua, target_id)?;
                let observer_table = create_entity_table(lua, observer_id)?;
                callback.call((target_table, observer_table))?;

                Ok(())
            });
        });

        entity.delete_callbacks.push(callback);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(FIELD_TABLE, "callback_on_delete", |api_ctx, lua, params| {
        let (_, id, callback): (rollback_mlua::Table, EntityID, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let entities = &mut api_ctx.simulation.entities;
        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let vm_index = api_ctx.vm_index;

        let callback = BattleCallback::new_transformed_lua_callback(
            lua,
            vm_index,
            callback,
            move |_, lua, _| {
                let entity_table = create_entity_table(lua, id)?;
                lua.pack_multi(entity_table)
            },
        )?;

        entity.delete_callbacks.push(callback);

        lua.pack_multi(())
    });
}

fn generate_find_entity_fn<Q: hecs::Query>(lua_api: &mut BattleLuaApi, name: &str) {
    lua_api.add_dynamic_function(FIELD_TABLE, name, |api_ctx, lua, params| {
        let (_, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let tables = {
            // scope to prevent RefCell borrow escaping when control is passed back to lua
            let mut api_ctx = api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;

            let mut tables: Vec<rollback_mlua::Table> = Vec::with_capacity(entities.len() as usize);

            for (id, entity) in entities.query_mut::<hecs::With<&Entity, Q>>() {
                if !entity.deleted && entity.on_field {
                    tables.push(create_entity_table(lua, id.into())?);
                }
            }

            tables
        };

        let filtered_tables: Vec<rollback_mlua::Table> = tables
            .into_iter()
            .filter(|table| {
                callback.call::<_, bool>(table.clone()).unwrap_or_else(|e| {
                    log::error!("{e}");
                    false
                })
            })
            .collect();

        lua.pack_multi(filtered_tables)
    });
}

fn generate_find_hittable_fn<Q: hecs::Query>(lua_api: &mut BattleLuaApi, name: &str) {
    lua_api.add_dynamic_function(FIELD_TABLE, name, |api_ctx, lua, params| {
        let (_, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let tables = {
            // scope to prevent RefCell borrow escaping when control is passed back to lua
            let mut api_ctx = api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;

            let mut tables: Vec<rollback_mlua::Table> = Vec::with_capacity(entities.len() as usize);

            for (id, (entity, living)) in entities.query_mut::<hecs::With<(&Entity, &Living), Q>>()
            {
                if !entity.deleted && entity.on_field && living.hitbox_enabled {
                    tables.push(create_entity_table(lua, id.into())?);
                }
            }

            tables
        };

        let filtered_tables: Vec<rollback_mlua::Table> = tables
            .into_iter()
            .filter(|table| {
                callback.call::<_, bool>(table.clone()).unwrap_or_else(|e| {
                    log::error!("{e}");
                    false
                })
            })
            .collect();

        lua.pack_multi(filtered_tables)
    });
}

fn generate_find_nearest_hittable_fn<Q: hecs::Query>(lua_api: &mut BattleLuaApi, name: &str) {
    lua_api.add_dynamic_function(FIELD_TABLE, name, |api_ctx, lua, params| {
        let (_, ref_table, callback): (
            rollback_mlua::Table,
            rollback_mlua::Table,
            rollback_mlua::Function,
        ) = lua.unpack_multi(params)?;

        let tables = {
            // scope to prevent RefCell borrow escaping when control is passed back to lua
            let mut api_ctx = api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;

            let ref_id: EntityID = ref_table.raw_get("#id")?;
            let ref_entity = entities
                .query_one_mut::<&Entity>(ref_id.into())
                .map_err(|_| entity_not_found())?;

            let ref_pos = IVec2::new(ref_entity.x, ref_entity.y);

            let mut tables: Vec<(rollback_mlua::Table, IVec2)> =
                Vec::with_capacity(entities.len() as usize);

            for (id, (entity, living)) in entities.query_mut::<hecs::With<(&Entity, &Living), Q>>()
            {
                let pos = IVec2::new(entity.x, entity.y);

                if !entity.deleted && entity.on_field && living.hitbox_enabled {
                    tables.push((create_entity_table(lua, id.into())?, pos));
                }
            }

            tables.sort_by_cached_key(|(_, pos)| {
                let diff = (ref_pos - *pos).abs();

                diff.x + diff.y
            });

            tables
        };

        let filtered_tables: Vec<rollback_mlua::Table> = tables
            .into_iter()
            .map(|(table, _)| table)
            .filter(|table| {
                callback.call::<_, bool>(table.clone()).unwrap_or_else(|e| {
                    log::error!("{e}");
                    false
                })
            })
            .collect();

        lua.pack_multi(filtered_tables)
    });
}

pub fn get_field_table(lua: &rollback_mlua::Lua) -> rollback_mlua::Result<rollback_mlua::Table> {
    lua.named_registry_value(FIELD_TABLE)
}
