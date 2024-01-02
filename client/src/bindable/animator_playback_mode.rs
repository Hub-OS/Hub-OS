use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(Clone, Copy, PartialEq, Eq, FromPrimitive)]
pub enum AnimatorPlaybackMode {
    Once,
    Loop,
    Bounce,
    Reverse,
}

impl<'lua> rollback_mlua::FromLua<'lua> for AnimatorPlaybackMode {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        use num_traits::FromPrimitive;

        let number = match lua_value {
            rollback_mlua::Value::Number(number) => number as u8,
            rollback_mlua::Value::Integer(number) => number as u8,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "AnimatorPlaybackMode",
                    message: None,
                })
            }
        };

        AnimatorPlaybackMode::from_u8(number).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "AnimatorPlaybackMode",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for AnimatorPlaybackMode {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Integer(self as _))
    }
}
