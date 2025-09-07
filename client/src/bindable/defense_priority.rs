use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(PartialEq, Eq, Clone, Copy, FromPrimitive, PartialOrd, Ord)]
pub enum DefensePriority {
    Internal,
    Intangible,
    Barrier,
    Action,
    Body,
    Trap,
    Last, // special case, appends instead of replaces
}

impl<'lua> rollback_mlua::FromLua<'lua> for DefensePriority {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        use num_traits::FromPrimitive;

        let number = match lua_value {
            rollback_mlua::Value::Integer(number) => number as u8,
            rollback_mlua::Value::Number(number) => number as u8,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "DefensePriority",
                    message: None,
                })
            }
        };

        DefensePriority::from_u8(number).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "DefensePriority",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for DefensePriority {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Integer(
            self as u8 as rollback_mlua::Integer,
        ))
    }
}
