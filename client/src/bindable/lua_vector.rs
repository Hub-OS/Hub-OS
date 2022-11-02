use framework::prelude::Vec2;

pub struct LuaVector {
    pub x: f32,
    pub y: f32,
}

impl From<Vec2> for LuaVector {
    fn from(v: Vec2) -> Self {
        Self { x: v.x, y: v.y }
    }
}

impl From<LuaVector> for Vec2 {
    fn from(v: LuaVector) -> Self {
        Self::new(v.x, v.y)
    }
}

impl From<(f32, f32)> for LuaVector {
    fn from((x, y): (f32, f32)) -> Self {
        Self { x, y }
    }
}

impl From<LuaVector> for (f32, f32) {
    fn from(v: LuaVector) -> Self {
        (v.x, v.y)
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for LuaVector {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "LuaVector",
                    message: None,
                })
            }
        };

        Ok(LuaVector {
            x: table.get("x").ok().unwrap_or_default(),
            y: table.get("y").ok().unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for LuaVector {
    fn to_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;

        table.set("x", self.x)?;
        table.set("y", self.y)?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
