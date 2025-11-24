use super::errors::component_not_found;
use super::{BattleLuaApi, COMPONENT_TABLE, INIT_FN, UPDATE_FN};
use crate::battle::*;
use crate::bindable::*;

pub fn inject_component_api(lua_api: &mut BattleLuaApi) {
    // constructor in entity_api.rs Entity:create_component

    lua_api.add_dynamic_function(COMPONENT_TABLE, "owner", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let entity_table: rollback_mlua::Table = table.get("#entity")?;

        lua.pack_multi(entity_table)
    });

    lua_api.add_dynamic_function(COMPONENT_TABLE, "eject", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        Component::eject(api_ctx.simulation, id);

        lua.pack_multi(())
    });

    callback_setter(
        lua_api,
        UPDATE_FN,
        |component| &mut component.update_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    lua_api.add_dynamic_setter(COMPONENT_TABLE, INIT_FN, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let component = (api_ctx.simulation.components)
            .get_mut(id)
            .ok_or_else(component_not_found)?;

        let Some(callback) = callback else {
            component.init_callback = BattleCallback::default();
            return lua.pack_multi(());
        };

        let key = lua.create_registry_value(table)?;
        component.init_callback = BattleCallback::new_transformed_lua_callback(
            lua,
            api_ctx.vm_index,
            callback,
            move |_, lua, _| {
                let table: rollback_mlua::Table = lua.registry_value(&key)?;
                lua.pack_multi(table)
            },
        )?;

        let entities = &mut api_ctx.simulation.entities;

        if let Ok(entity) = entities.query_one_mut::<&Entity>(component.entity.into())
            && entity.spawned
        {
            let callback = component.init_callback.clone();
            callback.call(api_ctx.game_io, api_ctx.resources, api_ctx.simulation, ());
        }

        lua.pack_multi(())
    });
}

fn callback_setter<P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: for<'lua> fn(&mut Component) -> &mut BattleCallback<P, R>,
    param_transformer: for<'lua> fn(
        &'lua rollback_mlua::Lua,
        rollback_mlua::Table<'lua>,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + 'static,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    lua_api.add_dynamic_setter(COMPONENT_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let Some(component) = api_ctx.simulation.components.get_mut(id) else {
            return lua.pack_multi(());
        };

        if let Some(callback) = callback {
            let key = lua.create_registry_value(table)?;

            *callback_getter(component) = BattleCallback::new_transformed_lua_callback(
                lua,
                api_ctx.vm_index,
                callback,
                move |_, lua, p| {
                    let table: rollback_mlua::Table = lua.registry_value(&key)?;
                    param_transformer(lua, table, p)
                },
            )?;
        } else {
            *callback_getter(component) = BattleCallback::default();
        }

        lua.pack_multi(())
    });
}
