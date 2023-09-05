use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(Debug, Clone, Copy, FromPrimitive)]
pub enum Comparison {
    LT,
    LE,
    EQ,
    NE,
    GT,
    GE,
}

impl Comparison {
    pub fn compare<T: PartialOrd + PartialEq>(&self, a: T, b: T) -> bool {
        match self {
            Comparison::LT => a < b,
            Comparison::LE => a <= b,
            Comparison::EQ => a == b,
            Comparison::NE => a != b,
            Comparison::GT => a > b,
            Comparison::GE => a >= b,
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for Comparison {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        use num_traits::FromPrimitive;

        let number = match lua_value {
            rollback_mlua::Value::Number(number) => number as rollback_mlua::Integer,
            rollback_mlua::Value::Integer(number) => number,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "Compare",
                    message: None,
                })
            }
        };

        Comparison::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "Compare",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for Comparison {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Integer(
            self as rollback_mlua::Integer,
        ))
    }
}
