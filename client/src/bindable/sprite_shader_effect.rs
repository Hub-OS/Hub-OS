use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(Default, Clone, Copy, PartialEq, Eq, FromPrimitive)]
pub enum SpriteShaderEffect {
    #[default]
    Default,
    Greyscale,
}

impl<'lua> rollback_mlua::FromLua<'lua> for SpriteShaderEffect {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        use num_traits::FromPrimitive;

        let number = match lua_value {
            rollback_mlua::Value::Number(number) => number,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "SpriteShaderEffect",
                    message: None,
                })
            }
        };

        SpriteShaderEffect::from_u8(number as u8).ok_or(
            rollback_mlua::Error::FromLuaConversionError {
                from: lua_value.type_name(),
                to: "SpriteShaderEffect",
                message: None,
            },
        )
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for SpriteShaderEffect {
    fn to_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Number(self as u8 as f64))
    }
}
