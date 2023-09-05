use super::errors::entity_not_found;
use super::{
    create_entity_table, BattleLuaApi, DELETE_FN, HIT_FLAG_TABLE, HIT_HELPER_TABLE, STATUS_TABLE,
};
use crate::battle::{BattleCallback, Living};
use crate::bindable::{EntityId, HitFlag, HitFlags};
use crate::lua_api::helpers::inherit_metatable;
use crate::render::FrameTime;

pub fn inject_status_api(lua_api: &mut BattleLuaApi) {
    inject_hit_flag_api(lua_api);

    lua_api.add_dynamic_function(STATUS_TABLE, "owner", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        lua.pack_multi(table.raw_get::<_, rollback_mlua::Table>("#entity")?)
    });

    lua_api.add_dynamic_function(STATUS_TABLE, "remaining_time", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let entity_id: EntityId = table.raw_get("#entity_id")?;
        let flag: HitFlags = table.raw_get("#flag")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let living = entities
            .query_one_mut::<&mut Living>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let remaining_time = living.status_director.remaining_status_time(flag);
        lua.pack_multi(remaining_time)
    });

    lua_api.add_dynamic_function(
        STATUS_TABLE,
        "set_remaining_time",
        |api_ctx, lua, params| {
            let (table, duration): (rollback_mlua::Table, FrameTime) = lua.unpack_multi(params)?;

            let entity_id: EntityId = table.raw_get("#entity_id")?;
            let flag: HitFlags = table.raw_get("#flag")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;
            let living = entities
                .query_one_mut::<&mut Living>(entity_id.into())
                .map_err(|_| entity_not_found())?;

            living
                .status_director
                .set_remaining_status_time(flag, duration);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_setter(STATUS_TABLE, DELETE_FN, |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = table.raw_get("#entity_id")?;
        let flag: HitFlags = table.raw_get("#flag")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let living = entities
            .query_one_mut::<&mut Living>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let key = lua.create_registry_value(table)?;

        let destructor = callback
            .map(|callback| {
                BattleCallback::new_transformed_lua_callback(
                    lua,
                    api_ctx.vm_index,
                    callback,
                    move |_, lua, _| {
                        let table = lua.registry_value::<rollback_mlua::Table>(&key)?;
                        lua.pack_multi(table)
                    },
                )
            })
            .transpose()?;

        living.status_director.set_destructor(flag, destructor);

        lua.pack_multi(())
    });
}

pub fn inject_hit_flag_api(lua_api: &mut BattleLuaApi) {
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

                    let helper_table: rollback_mlua::Table = lua.globals().get(HIT_HELPER_TABLE)?;
                    let resolve_flag: rollback_mlua::Function = helper_table.get("resolve_flag")?;
                    let value: rollback_mlua::Value = resolve_flag.call(key.clone())?;

                    table.set(key, value.clone())?;

                    Ok(value)
                },
            )?,
        )?;

        let hit_flags_table = lua.create_table()?;
        hit_flags_table.set_metatable(Some(meta_table));
        globals.set(HIT_FLAG_TABLE, hit_flags_table)?;

        Ok(())
    });

    lua_api.add_dynamic_function(HIT_HELPER_TABLE, "resolve_flag", |api_ctx, lua, params| {
        let key: rollback_mlua::String = lua.unpack_multi(params)?;
        let key_str = key.to_str()?;

        let api_ctx = api_ctx.borrow();
        let status_registry = &api_ctx.resources.status_registry;

        lua.pack_multi(HitFlag::from_str(status_registry, key_str))
    })
}

pub fn create_status_table(
    lua: &rollback_mlua::Lua,
    id: EntityId,
    flag: HitFlags,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    inherit_metatable(lua, STATUS_TABLE, &table)?;

    table.raw_set("#entity", create_entity_table(lua, id)?)?;
    table.raw_set("#entity_id", id)?;
    table.raw_set("#flag", flag)?;

    Ok(table)
}
