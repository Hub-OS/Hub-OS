use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(PartialEq, Eq, Default, Clone, Copy, FromPrimitive)]
pub enum CharacterRank {
    #[default]
    V1,
    V2,
    V3,
    V4,
    V5,
    SP,
    EX,
    Rare1,
    Rare2,
    NM,
    RV,
    DS,
    Alpha,
    Beta,
    Omega,
    Sigma,
}

impl CharacterRank {
    pub fn suffix(self) -> &'static str {
        match self {
            Self::V2 => "2", // alternate: "\u{e005}"
            Self::V3 => "3", // alternate: "\u{e006}"
            Self::V4 => "4", // alternate: "\u{e007}"
            Self::V5 => "5", // alternate: "\u{e008}"
            Self::EX => "\u{e000}",
            Self::SP => "\u{e001}",
            Self::DS => "\u{e003}",
            Self::RV => "\u{e004}",
            Self::NM => "\u{e005}",
            Self::Alpha => "α",
            Self::Beta => "β",
            Self::Omega => "Ω",
            Self::Sigma => "Σ",
            _ => "",
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for CharacterRank {
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
                    to: "CharacterRank",
                    message: None,
                })
            }
        };

        CharacterRank::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "CharacterRank",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for CharacterRank {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Number(self as u8 as f64))
    }
}
