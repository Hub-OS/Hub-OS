use packets::address_parsing::{uri_decode_raw, uri_encode_raw};

use super::LuaApi;

pub fn inject_static(lua_api: &mut LuaApi) {
    lua_api.add_static_injector(|lua| {
        let globals = lua.globals();

        let net_table: mlua::Table = globals.get("Net")?;

        net_table.set(
            "encode_uri_component",
            lua.create_function(|_lua, text: mlua::String| Ok(uri_encode_raw(text.as_bytes())))?,
        )?;

        net_table.set(
            "decode_uri_component",
            lua.create_function(|_lua, text: mlua::String| Ok(uri_decode_raw(text.to_str()?)))?,
        )?;

        Ok(())
    });
}
