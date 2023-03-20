use super::errors::{augment_not_found, entity_not_found};
use super::{
    create_entity_table, BattleLuaApi, AUGMENT_TABLE, CHARGED_ATTACK_FN, CHARGE_TIMING_FN,
    DELETE_FN, NORMAL_ATTACK_FN, SPECIAL_ATTACK_FN,
};
use crate::battle::{Augment, BattleCallback, Player};
use crate::bindable::{EntityId, GenerationalIndex};
use crate::lua_api::helpers::inherit_metatable;

pub fn inject_augment_api(lua_api: &mut BattleLuaApi) {
    getter(lua_api, "get_id", |augment, _, _: ()| {
        Ok(augment.package_id.clone())
    });

    getter(lua_api, "get_level", |augment, _, _: ()| Ok(augment.level));

    lua_api.add_dynamic_function(AUGMENT_TABLE, "get_owner", move |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        lua.pack_multi(create_entity_table(lua, id)?)
    });

    callback_setter(
        lua_api,
        CHARGE_TIMING_FN,
        |augment| {
            augment.calculate_charge_time_callback = Some(BattleCallback::default());
            augment.calculate_charge_time_callback.as_mut().unwrap()
        },
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        NORMAL_ATTACK_FN,
        |augment: &mut Augment| {
            augment.normal_attack_callback = Some(BattleCallback::default());
            augment.normal_attack_callback.as_mut().unwrap()
        },
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        CHARGED_ATTACK_FN,
        |augment: &mut Augment| {
            augment.charged_attack_callback = Some(BattleCallback::default());
            augment.charged_attack_callback.as_mut().unwrap()
        },
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        SPECIAL_ATTACK_FN,
        |augment: &mut Augment| {
            augment.special_attack_callback = Some(BattleCallback::default());
            augment.special_attack_callback.as_mut().unwrap()
        },
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        DELETE_FN,
        |augment: &mut Augment| &mut augment.delete_callback,
        |lua, table, _| lua.pack_multi(table),
    );
}

pub fn create_augment_table(
    lua: &rollback_mlua::Lua,
    entity_id: EntityId,
    index: generational_arena::Index,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    table.raw_set("#id", entity_id)?;
    table.raw_set("#index", GenerationalIndex::from(index))?;

    inherit_metatable(lua, AUGMENT_TABLE, &table)?;

    Ok(table)
}

fn getter<F, P, R>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    R: for<'lua> rollback_mlua::ToLua<'lua>,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(&Augment, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<R> + 'static,
{
    lua_api.add_dynamic_function(AUGMENT_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let augment = player
            .augments
            .get(index.into())
            .ok_or_else(augment_not_found)?;

        lua.pack_multi(callback(augment, lua, param)?)
    });
}

fn setter<F, P, R>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    R: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(&mut Augment, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<R>
        + 'static,
{
    lua_api.add_dynamic_function(AUGMENT_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let augment = player
            .augments
            .get_mut(index.into())
            .ok_or_else(augment_not_found)?;

        lua.pack_multi(callback(augment, lua, param)?)
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
    G: for<'lua> Fn(&mut Augment) -> &mut BattleCallback<P, R> + Send + Sync + 'static,
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
    lua_api.add_dynamic_setter(AUGMENT_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let augment = player
            .augments
            .get_mut(index.into())
            .ok_or_else(augment_not_found)?;

        let key = lua.create_registry_value(table)?;

        if let Some(callback) = callback {
            *callback_getter(augment) = BattleCallback::new_transformed_lua_callback(
                lua,
                api_ctx.vm_index,
                callback,
                move |_, lua, p| {
                    let table: rollback_mlua::Table = lua.registry_value(&key)?;
                    param_transformer(lua, table, p)
                },
            )?;
        } else {
            *callback_getter(augment) = BattleCallback::default();
        }

        lua.pack_multi(())
    });
}
