use std::sync::Arc;

use super::errors::{ability_mod_not_found, entity_not_found};
use super::{
    create_entity_table, BattleLuaApi, ABILITY_MOD_TABLE, CHARGED_ATTACK_FN, DELETE_FN,
    NORMAL_ATTACK_FN, SPECIAL_ATTACK_FN,
};
use crate::battle::{AbilityModifier, BattleCallback, Player};
use crate::bindable::{EntityID, GenerationalIndex};
use crate::lua_api::helpers::inherit_metatable;

pub fn inject_ability_mod_api(lua_api: &mut BattleLuaApi) {
    getter(
        lua_api,
        "get_level",
        |modifier, _, _: ()| Ok(modifier.level),
    );

    lua_api.add_dynamic_function(ABILITY_MOD_TABLE, "get_owner", move |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityID = table.raw_get("#id")?;

        lua.pack_multi(create_entity_table(lua, id)?)
    });

    callback_setter(
        lua_api,
        NORMAL_ATTACK_FN,
        |modifier: &mut AbilityModifier| {
            modifier.normal_attack_callback = Some(BattleCallback::default());
            modifier.normal_attack_callback.as_mut().unwrap()
        },
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        CHARGED_ATTACK_FN,
        |modifier: &mut AbilityModifier| {
            modifier.charged_attack_callback = Some(BattleCallback::default());
            modifier.charged_attack_callback.as_mut().unwrap()
        },
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        SPECIAL_ATTACK_FN,
        |modifier: &mut AbilityModifier| {
            modifier.special_attack_callback = Some(BattleCallback::default());
            modifier.special_attack_callback.as_mut().unwrap()
        },
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        DELETE_FN,
        |modifier: &mut AbilityModifier| &mut modifier.delete_callback,
        |lua, table, _| lua.pack_multi(table),
    );
}

pub fn create_ability_mod_table(
    lua: &rollback_mlua::Lua,
    entity_id: EntityID,
    index: generational_arena::Index,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    table.raw_set("#id", entity_id)?;
    table.raw_set("#index", GenerationalIndex::from(index))?;

    inherit_metatable(lua, ABILITY_MOD_TABLE, &table)?;

    Ok(table)
}

fn getter<F, P, R>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    R: for<'lua> rollback_mlua::ToLua<'lua>,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(&AbilityModifier, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<R>
        + 'static,
{
    lua_api.add_dynamic_function(ABILITY_MOD_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityID = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let modifier = player
            .modifiers
            .get(index.into())
            .ok_or_else(ability_mod_not_found)?;

        lua.pack_multi(callback(modifier, lua, param)?)
    });
}

fn setter<F, P, R>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    R: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(&mut AbilityModifier, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<R>
        + 'static,
{
    lua_api.add_dynamic_function(ABILITY_MOD_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityID = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let modifier = player
            .modifiers
            .get_mut(index.into())
            .ok_or_else(ability_mod_not_found)?;

        lua.pack_multi(callback(modifier, lua, param)?)
    });
}

fn callback_setter<G, P, F, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: G,
    param_transformer: F,
) where
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default,
    G: for<'lua> Fn(&mut AbilityModifier) -> &mut BattleCallback<P, R> + Send + Sync + 'static,
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
    lua_api.add_dynamic_setter(ABILITY_MOD_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let id: EntityID = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let modifier = player
            .modifiers
            .get_mut(index.into())
            .ok_or_else(ability_mod_not_found)?;

        let key = Arc::new(lua.create_registry_value(table)?);

        *callback_getter(modifier) = BattleCallback::new_transformed_lua_callback(
            lua,
            api_ctx.vm_index,
            callback,
            move |_, lua, p| {
                let table: rollback_mlua::Table = lua.registry_value(&key)?;
                param_transformer(lua, table, p)
            },
        )?;

        lua.pack_multi(())
    });
}
