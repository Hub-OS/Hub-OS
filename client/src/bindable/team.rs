use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(Debug, PartialEq, Eq, Default, Clone, Copy, FromPrimitive)]
pub enum Team {
    #[default]
    Unset,
    Other,
    Red,
    Blue,
}

impl Team {
    pub fn flips_perspective(&self) -> bool {
        matches!(self, Team::Blue)
    }

    pub fn opposite(self) -> Team {
        match self {
            Team::Red => Team::Blue,
            Team::Blue => Team::Red,
            _ => self,
        }
    }

    pub fn is_allied(self, other: Self) -> bool {
        self == other && self != Team::Other
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for Team {
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
                    to: "Team",
                    message: None,
                });
            }
        };

        Team::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "Team",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for Team {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Number(self as u8 as f64))
    }
}
