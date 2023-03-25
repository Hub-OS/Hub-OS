use super::{BattleLuaApi, MOVEMENT_TABLE};
use crate::bindable::Movement;
use crate::lua_api::helpers::inherit_metatable;

pub fn inject_movement_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(MOVEMENT_TABLE, "new", |_, lua, _| {
        lua.pack_multi(create_movement_table(lua))
    });

    lua_api.add_dynamic_function(MOVEMENT_TABLE, "animation_progress", |_, lua, params| {
        let movement: Movement = lua.unpack_multi(params)?;

        lua.pack_multi(movement.animation_progress_percent())
    });

    lua_api.add_dynamic_function(MOVEMENT_TABLE, "is_sliding", |_, lua, params| {
        let movement: Movement = lua.unpack_multi(params)?;

        lua.pack_multi(movement.is_sliding())
    });

    lua_api.add_dynamic_function(MOVEMENT_TABLE, "is_jumping", |_, lua, params| {
        let movement: Movement = lua.unpack_multi(params)?;

        lua.pack_multi(movement.is_jumping())
    });

    lua_api.add_dynamic_function(MOVEMENT_TABLE, "is_teleporting", |_, lua, params| {
        let movement: Movement = lua.unpack_multi(params)?;

        lua.pack_multi(movement.is_teleporting())
    });
}

fn create_movement_table(lua: &rollback_mlua::Lua) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    inherit_metatable(lua, MOVEMENT_TABLE, &table)?;

    Ok(table)
}
