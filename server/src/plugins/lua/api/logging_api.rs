use super::LuaApi;

pub fn inject_static(lua_api: &mut LuaApi) {
    lua_api.add_static_injector(|lua| {
        let globals = lua.globals();

        globals.set(
            "print",
            lua.create_function(|_lua, args: mlua::MultiValue| {
                log::info!("{}", format_args(args));
                Ok(mlua::Value::Nil)
            })?,
        )?;

        globals.set(
            "printerr",
            lua.create_function(|_lua, args: mlua::MultiValue| {
                log::error!("{}", format_args(args));
                Ok(mlua::Value::Nil)
            })?,
        )?;

        globals.set(
            "warn",
            lua.create_function(|_lua, args: mlua::MultiValue| {
                log::warn!("{}", format_args(args));
                Ok(mlua::Value::Nil)
            })?,
        )?;

        globals.set(
            "tostring",
            lua.create_function(|_lua, value: mlua::Value| Ok(tostring(value)))?,
        )?;

        Ok(())
    });
}

fn format_args(args: mlua::MultiValue) -> String {
    args.into_iter()
        .map(tostring)
        .collect::<Vec<String>>()
        .join("\t")
}

fn tostring(value: mlua::Value) -> String {
    match value {
        mlua::Value::String(lua_string) => {
            String::from_utf8_lossy(lua_string.as_bytes()).to_string()
        }
        mlua::Value::Error(error) => format!("{}", error),
        _ => super::lua_helpers::lua_value_to_string(value, "\t", 0),
    }
}
