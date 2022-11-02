use super::{BattleLuaApi, BUSTER_TABLE};

pub fn inject_built_in_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(BUSTER_TABLE, "new", |_, lua, params| {
        let function = match lua.named_registry_value("buster")? {
            rollback_mlua::Value::Function(function) => function,
            _ => {
                let function = lua
                    .load(include_str!("built_in/buster.lua"))
                    .set_name("built_in/buster.lua")?
                    .into_function()?;

                lua.set_named_registry_value("buster", function.clone())?;

                function
            }
        };

        function.call(params)
    })
}
