use super::animation_api::create_animation_table;
use super::sprite_api::create_sprite_table;
use super::{BattleLuaApi, SYNC_NODE_TABLE};
use crate::bindable::{EntityId, GenerationalIndex};
use crate::lua_api::helpers::inherit_metatable;

pub fn inject_sync_node_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(SYNC_NODE_TABLE, "sprite", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;
        let sprite_table: rollback_mlua::Table = table.get("#sprite")?;

        lua.pack_multi(sprite_table)
    });

    lua_api.add_dynamic_function(SYNC_NODE_TABLE, "animation", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;
        let animation_table: rollback_mlua::Table = table.get("#animation")?;

        lua.pack_multi(animation_table)
    });
}

pub fn create_sync_node_table(
    lua: &rollback_mlua::Lua,
    entity_id: EntityId,
    sprite_index: GenerationalIndex,
    animator_index: generational_arena::Index,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    table.raw_set("#anim", GenerationalIndex::from(animator_index))?;
    table.raw_set("#animation", create_animation_table(lua, animator_index)?)?;
    table.raw_set(
        "#sprite",
        create_sprite_table(lua, entity_id, sprite_index, Some(animator_index))?,
    )?;

    inherit_metatable(lua, SYNC_NODE_TABLE, &table)?;

    Ok(table)
}
