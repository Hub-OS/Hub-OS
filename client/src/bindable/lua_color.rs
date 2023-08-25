use framework::prelude::Color;

#[derive(Clone)]
pub struct LuaColor {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

impl LuaColor {
    pub fn new(r: u8, g: u8, b: u8, a: u8) -> Self {
        Self { r, g, b, a }
    }
}

impl From<LuaColor> for Color {
    fn from(color: LuaColor) -> Self {
        Color::from((color.r, color.g, color.b, color.a))
    }
}

impl From<Color> for LuaColor {
    fn from(c: Color) -> Self {
        Self::new(
            (c.r * 255.0) as u8,
            (c.g * 255.0) as u8,
            (c.b * 255.0) as u8,
            (c.a * 255.0) as u8,
        )
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for LuaColor {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "LuaColor",
                    message: None,
                })
            }
        };

        Ok(LuaColor {
            r: table.get("r").ok().unwrap_or_default(),
            g: table.get("g").ok().unwrap_or_default(),
            b: table.get("b").ok().unwrap_or_default(),
            a: table.get("a").ok().unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for LuaColor {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;

        table.set("r", self.r)?;
        table.set("g", self.g)?;
        table.set("b", self.b)?;
        table.set("a", self.a)?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
