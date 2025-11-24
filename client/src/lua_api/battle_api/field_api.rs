use super::errors::entity_not_found;
use super::tile_api::create_tile_table;
use super::{BattleLuaApi, FIELD_TABLE, create_entity_table};
use crate::battle::{Character, Entity, Obstacle, Player, Spell};
use crate::bindable::{EntityId, Team};
use crate::lua_api::FIELD_COMPAT_TABLE;
use crate::render::FrameTime;
use framework::prelude::IVec2;

pub fn inject_field_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(FIELD_TABLE, "tile_at", |api_ctx, lua, params| {
        let (x, y): (i32, i32) = lua.unpack_multi(params)?;

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
        let (entity_table, rest): (rollback_mlua::Table, rollback_mlua::MultiValue) =
            lua.unpack_multi(params)?;

        let (x, y): (i32, i32) = lua.unpack_multi(rest.clone()).or_else(
            move |_| -> rollback_mlua::Result<(i32, i32)> {
                let tile_table: rollback_mlua::Table = lua.unpack_multi(rest)?;

                let x = tile_table.raw_get("#x")?;
                let y = tile_table.raw_get("#y")?;

                Ok((x, y))
            },
        )?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let id: EntityId = entity_table.raw_get("#id")?;

        if !api_ctx.simulation.field.in_bounds((x, y)) {
            Entity::delete(api_ctx.game_io, api_ctx.resources, api_ctx.simulation, id);
            return lua.pack_multi(());
        }

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
        let id: EntityId = lua.unpack_multi(params)?;

        let api_ctx = api_ctx.borrow();

        if !api_ctx.simulation.entities.contains(id.into()) {
            return lua.pack_multi(());
        }

        lua.pack_multi(create_entity_table(lua, id)?)
    });

    generate_find_entity_fn::<()>(lua_api, "find_entities");
    generate_find_entity_fn::<&Character>(lua_api, "find_characters");
    generate_find_entity_fn::<&Obstacle>(lua_api, "find_obstacles");
    generate_find_entity_fn::<&Player>(lua_api, "find_players");
    generate_find_entity_fn::<hecs::Without<&Spell, &Obstacle>>(lua_api, "find_spells");

    generate_find_nearest_fn::<&Character>(lua_api, "find_nearest_characters");
    generate_find_nearest_fn::<&Player>(lua_api, "find_nearest_players");

    lua_api.add_dynamic_function(FIELD_TABLE, "find_tiles", |api_ctx, lua, params| {
        let callback: rollback_mlua::Function = lua.unpack_multi(params)?;

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

    lua_api.add_dynamic_function(FIELD_TABLE, "shake", |api_ctx, lua, params| {
        let (power, duration): (f32, FrameTime) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();

        let camera = &mut api_ctx.simulation.camera;
        camera.shake(power, duration);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(FIELD_TABLE, "reclaim_column", |api_ctx, lua, params| {
        let (x, team): (i32, Team) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();

        let field = &mut api_ctx.simulation.field;

        for y in 0..field.rows() as i32 {
            if let Some(tile) = field.tile_at_mut((x, y))
                && tile.original_team() == team
            {
                tile.set_team_reclaim_timer(0);
            }
        }

        lua.pack_multi(())
    });
}

fn generate_find_entity_fn<Q: hecs::Query>(lua_api: &mut BattleLuaApi, name: &str) {
    lua_api.add_dynamic_function(FIELD_TABLE, name, |api_ctx, lua, params| {
        let callback: rollback_mlua::Function = lua.unpack_multi(params)?;

        let tables = {
            // scope to prevent RefCell borrow escaping when control is passed back to lua
            let mut api_ctx = api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;

            let mut tables: Vec<rollback_mlua::Table> = Vec::with_capacity(entities.len() as usize);

            for (id, entity) in entities.query_mut::<hecs::With<&Entity, Q>>() {
                if entity.spawned && !entity.deleted {
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

fn generate_find_nearest_fn<Q: hecs::Query>(lua_api: &mut BattleLuaApi, name: &str) {
    lua_api.add_dynamic_function(FIELD_TABLE, name, |api_ctx, lua, params| {
        let (ref_table, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let tables = {
            // scope to prevent RefCell borrow escaping when control is passed back to lua
            let mut api_ctx = api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;

            let ref_id: EntityId = ref_table.raw_get("#id")?;
            let ref_entity = entities
                .query_one_mut::<&Entity>(ref_id.into())
                .map_err(|_| entity_not_found())?;

            let ref_pos = IVec2::new(ref_entity.x, ref_entity.y);

            let mut tables: Vec<(rollback_mlua::Table, IVec2)> =
                Vec::with_capacity(entities.len() as usize);

            for (id, entity) in entities.query_mut::<hecs::With<&Entity, Q>>() {
                let pos = IVec2::new(entity.x, entity.y);

                if entity.spawned && !entity.deleted {
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

pub fn get_field_compat_table(
    lua: &rollback_mlua::Lua,
) -> rollback_mlua::Result<rollback_mlua::Table<'_>> {
    if let Ok(table) = lua.named_registry_value::<rollback_mlua::Table>(FIELD_COMPAT_TABLE) {
        return Ok(table);
    }

    let source_name = lua
        .inspect_stack(1)
        .map(|debug| debug.source().source.unwrap_or_default().to_string())
        .unwrap_or_default();

    log::warn!("deprecated use of :field() in {source_name:?}");

    let table = lua.create_table()?;
    let metatable = lua.create_table()?;

    metatable.raw_set(
        "__index",
        lua.create_function(
            move |lua, (self_table, key): (rollback_mlua::Table, rollback_mlua::String)| {
                let value = self_table.raw_get::<_, rollback_mlua::Value>(key.clone())?;

                if !value.is_nil() {
                    return lua.pack_multi(value);
                }

                let table: rollback_mlua::Table = lua.named_registry_value(FIELD_TABLE)?;
                let value = table.get::<_, rollback_mlua::Value>(key.clone())?;

                if !value.is_function() {
                    return lua.pack_multi(value);
                }

                // create a wrapper function that strips `self``
                let captured_key = key.to_str()?.to_string();

                let wrapper_func =
                    lua.create_function(move |lua, mut params: rollback_mlua::MultiValue| {
                        params.pop_front();

                        let table: rollback_mlua::Table = lua.named_registry_value(FIELD_TABLE)?;
                        let func =
                            table.get::<_, rollback_mlua::Function>(captured_key.as_str())?;

                        func.call::<_, rollback_mlua::MultiValue>(params)
                    })?;

                self_table.raw_set(key, wrapper_func.clone())?;

                lua.pack_multi(wrapper_func)
            },
        )?,
    )?;

    table.set_metatable(Some(metatable));

    lua.set_named_registry_value(FIELD_COMPAT_TABLE, table.clone())?;
    Ok(table)
}
