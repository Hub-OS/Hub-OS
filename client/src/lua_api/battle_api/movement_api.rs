use super::tile_api::{create_tile_table, tile_mut_from_table};
use super::{BattleLuaApi, MOVEMENT_TABLE};
use crate::battle::Movement;
use crate::lua_api::helpers::inherit_metatable;
use crate::render::FrameTime;

pub fn inject_movement_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(MOVEMENT_TABLE, "new_teleport", |api_ctx, lua, params| {
        let dest_tile_table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, dest_tile_table)?;
        let movement = Movement::teleport(tile.position());

        lua.pack_multi(create_movement_table(lua, &movement)?)
    });

    lua_api.add_dynamic_function(MOVEMENT_TABLE, "new_slide", |api_ctx, lua, params| {
        let (dest_tile_table, duration): (rollback_mlua::Table, FrameTime) =
            lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, dest_tile_table)?;
        let movement = Movement::slide(tile.position(), duration);

        lua.pack_multi(create_movement_table(lua, &movement)?)
    });

    lua_api.add_dynamic_function(MOVEMENT_TABLE, "new_jump", |api_ctx, lua, params| {
        let (dest_tile_table, height, duration): (rollback_mlua::Table, f32, FrameTime) =
            lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, dest_tile_table)?;
        let movement = Movement::jump(tile.position(), height, duration);

        lua.pack_multi(create_movement_table(lua, &movement)?)
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

pub fn create_movement_table<'lua>(
    lua: &'lua rollback_mlua::Lua,
    movement: &Movement,
) -> rollback_mlua::Result<rollback_mlua::Table<'lua>> {
    let table = lua.create_table()?;
    inherit_metatable(lua, MOVEMENT_TABLE, &table)?;

    table.raw_set("dest_tile", create_tile_table(lua, movement.dest)?)?;
    table.raw_set("elapsed", movement.elapsed)?;
    table.raw_set("delta", movement.delta)?;
    table.raw_set("delay", movement.delay)?;
    table.raw_set("endlag", movement.endlag)?;
    table.raw_set("height", movement.height)?;

    Ok(table)
}
