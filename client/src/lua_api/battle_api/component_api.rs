use super::errors::{component_not_found, entity_not_found};
use super::{BattleLuaApi, COMPONENT_TABLE, INIT_FN, UPDATE_FN};
use crate::battle::*;
use crate::bindable::*;

pub fn inject_component_api(lua_api: &mut BattleLuaApi) {
    // constructor in entity_api.rs Entity:create_component

    lua_api.add_dynamic_function(COMPONENT_TABLE, "get_owner", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let entity_table: rollback_mlua::Table = table.get("#entity")?;

        lua.pack_multi(entity_table)
    });

    lua_api.add_dynamic_function(COMPONENT_TABLE, "eject", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let component = api_ctx
            .simulation
            .components
            .remove(id.into())
            .ok_or_else(component_not_found)?;

        if component.lifetime == ComponentLifetime::Local {
            let entities = &mut api_ctx.simulation.entities;

            let entity = entities
                .query_one_mut::<&mut Entity>(component.entity.into())
                .map_err(|_| entity_not_found())?;

            let arena_index: generational_arena::Index = id.into();
            let index = (entity.local_components)
                .iter()
                .position(|stored_index| *stored_index == arena_index)
                .unwrap();

            entity.local_components.swap_remove(index);
        }

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
            .get_mut(id.into())
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

        if let Ok(entity) = entities.query_one_mut::<&Entity>(component.entity.into()) {
            if entity.spawned {
                let callback = component.init_callback.clone();
                callback.call(api_ctx.game_io, api_ctx.simulation, api_ctx.vms, ());
            }
        }

        lua.pack_multi(())
    });
}

fn callback_setter<G, P, F, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: G,
    param_transformer: F,
) where
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
    G: for<'lua> Fn(&mut Component) -> &mut BattleCallback<P, R> + Send + Sync + 'static,
    F: for<'lua> Fn(
            &'lua rollback_mlua::Lua,
            rollback_mlua::Table<'lua>,
            P,
        ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>
        + Send
        + Sync
        + Copy
        + 'static,
{
    lua_api.add_dynamic_setter(COMPONENT_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let component = (api_ctx.simulation.components)
            .get_mut(id.into())
            .ok_or_else(component_not_found)?;

        let key = lua.create_registry_value(table)?;

        if let Some(callback) = callback {
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
