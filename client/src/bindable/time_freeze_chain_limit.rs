#[derive(Clone, PartialEq, Eq)]
pub enum TimeFreezeChainLimit {
    PerTeam(u8),
    PerEntity(u8),
    Unlimited,
}

impl Default for TimeFreezeChainLimit {
    fn default() -> Self {
        TimeFreezeChainLimit::PerTeam(1)
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for TimeFreezeChainLimit {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let number = match lua_value {
            rollback_mlua::Value::Number(number) => number as u64,
            rollback_mlua::Value::Integer(number) => number as u64,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "TimeFreezeChainLimit",
                    message: None,
                });
            }
        };

        let variant = number & 255;

        let limit = match variant {
            0 => TimeFreezeChainLimit::PerTeam((number >> 8) as _),
            1 => TimeFreezeChainLimit::PerEntity((number >> 8) as _),
            2 => TimeFreezeChainLimit::Unlimited,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "TimeFreezeChainLimit",
                    message: None,
                });
            }
        };

        Ok(limit)
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for TimeFreezeChainLimit {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let i = match self {
            TimeFreezeChainLimit::PerTeam(n) => (n as i64) << 8,
            TimeFreezeChainLimit::PerEntity(n) => 1 | ((n as i64) << 8),
            TimeFreezeChainLimit::Unlimited => 2,
        };

        Ok(rollback_mlua::Value::Integer(i))
    }
}
