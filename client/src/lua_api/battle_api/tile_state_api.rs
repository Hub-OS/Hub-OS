use super::errors::invalid_custom_tile_state;
use super::field_api::get_field_compat_table;
use super::tile_api::create_tile_table;
use super::{
    BattleLuaApi, CAN_REPLACE_FN, CUSTOM_TILE_STATE_TABLE, ENTITY_ENTER_FN, ENTITY_LEAVE_FN,
    ENTITY_STOP_FN, REPLACE_FN, TILE_STATE_TABLE, TILE_TABLE, UPDATE_FN, create_entity_table,
};
use crate::battle::{BattleCallback, TileState};
use crate::lua_api::helpers::inherit_metatable;

pub fn inject_tile_state_api(lua_api: &mut BattleLuaApi) {
    inject_custom_tile_state_api(lua_api);

    lua_api.add_static_injector(|lua| {
        let globals = lua.globals();

        let meta_table = lua.create_table()?;
        meta_table.set(
            "__index",
            lua.create_function(
                |lua, (table, key): (rollback_mlua::Table, rollback_mlua::String)| {
                    let stored_value: Option<rollback_mlua::Value> = table.raw_get(key.clone())?;

                    if let Some(value) = stored_value {
                        return Ok(value);
                    }

                    let tile_table: rollback_mlua::Table = lua.globals().get(TILE_TABLE)?;
                    let resolve_state: rollback_mlua::Function = tile_table.get("resolve_state")?;
                    let value: rollback_mlua::Value = resolve_state.call(key.clone())?;

                    table.raw_set(key, value.clone())?;

                    Ok(value)
                },
            )?,
        )?;

        let tile_state_table = lua.create_table()?;
        tile_state_table.set_metatable(Some(meta_table));
        globals.set(TILE_STATE_TABLE, tile_state_table)?;

        Ok(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "resolve_state", |api_ctx, lua, params| {
        let key: rollback_mlua::String = lua.unpack_multi(params)?;
        let key_str = key.to_str()?;

        let api_ctx = api_ctx.borrow();
        let tile_states = &api_ctx.simulation.tile_states;

        let index = tile_states
            .iter()
            .position(|state| state.state_name == key_str)
            .map(|i| i as i64)
            .unwrap_or(-1);

        lua.pack_multi(index)
    });
}

fn inject_custom_tile_state_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(CUSTOM_TILE_STATE_TABLE, "field", |_, lua, _| {
        let field_table = get_field_compat_table(lua)?;

        lua.pack_multi(field_table)
    });

    callback_setter(
        lua_api,
        CAN_REPLACE_FN,
        |tile_state| &mut tile_state.can_replace_callback,
        |lua, table, (x, y, state_index)| {
            lua.pack_multi((table, create_tile_table(lua, (x, y))?, state_index))
        },
    );

    callback_setter(
        lua_api,
        REPLACE_FN,
        |tile_state| &mut tile_state.replace_callback,
        |lua, table, (x, y)| lua.pack_multi((table, create_tile_table(lua, (x, y))?)),
    );

    callback_setter(
        lua_api,
        UPDATE_FN,
        |tile_state| &mut tile_state.update_callback,
        |lua, table, tile_position| lua.pack_multi((table, create_tile_table(lua, tile_position)?)),
    );

    callback_setter(
        lua_api,
        ENTITY_ENTER_FN,
        |tile_state| &mut tile_state.entity_enter_callback,
        |lua, table, id| lua.pack_multi((table, create_entity_table(lua, id)?)),
    );

    callback_setter(
        lua_api,
        ENTITY_LEAVE_FN,
        |tile_state| &mut tile_state.entity_leave_callback,
        |lua, table, (id, old_x, old_y)| {
            lua.pack_multi((
                table,
                create_entity_table(lua, id)?,
                create_tile_table(lua, (old_x, old_y))?,
            ))
        },
    );

    callback_setter(
        lua_api,
        ENTITY_STOP_FN,
        |tile_state| &mut tile_state.entity_stop_callback,
        |lua, table, (id, old_x, old_y)| {
            lua.pack_multi((
                table,
                create_entity_table(lua, id)?,
                create_tile_table(lua, (old_x, old_y))?,
            ))
        },
    );
}

fn callback_setter<P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: for<'lua> fn(&mut TileState) -> &mut BattleCallback<P, R>,
    param_transformer: for<'lua> fn(
        &'lua rollback_mlua::Lua,
        rollback_mlua::Table<'lua>,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + 'static,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    lua_api.add_dynamic_setter(
        CUSTOM_TILE_STATE_TABLE,
        name,
        move |api_ctx, lua, params| {
            let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
                lua.unpack_multi(params)?;

            let index: usize = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let registry = &mut api_ctx.simulation.tile_states;

            let Some(custom_state) = registry.get_mut(index) else {
                return Err(invalid_custom_tile_state());
            };

            let key = lua.create_registry_value(table)?;

            *callback_getter(custom_state) = callback
                .map(|callback| {
                    BattleCallback::new_transformed_lua_callback(
                        lua,
                        api_ctx.vm_index,
                        callback,
                        move |_, lua, p| {
                            let table: rollback_mlua::Table = lua.registry_value(&key)?;
                            param_transformer(lua, table, p)
                        },
                    )
                })
                .transpose()?
                .unwrap_or_default();

            lua.pack_multi(())
        },
    );
}

pub fn create_custom_tile_state_table(
    lua: &rollback_mlua::Lua,
    index: usize,
) -> rollback_mlua::Result<rollback_mlua::Table<'_>> {
    let table = lua.create_table()?;
    inherit_metatable(lua, CUSTOM_TILE_STATE_TABLE, &table)?;

    table.raw_set("#id", index)?;

    Ok(table)
}
