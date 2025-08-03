use num_derive::FromPrimitive;

#[repr(u8)]
#[derive(Default, Debug, Copy, Clone, PartialEq, PartialOrd, Ord, Eq, FromPrimitive)]
pub enum CardClass {
    #[default]
    Standard,
    Mega,
    Giga,
    Dark,
    Recipe,
}

impl CardClass {
    pub fn translation_key(self) -> &'static str {
        match self {
            CardClass::Standard => "card-class-standard",
            CardClass::Mega => "card-class-mega",
            CardClass::Giga => "card-class-giga",
            CardClass::Dark => "card-class-dark",
            CardClass::Recipe => "card-class-recipe",
        }
    }
}

impl From<String> for CardClass {
    fn from(s: String) -> Self {
        Self::from(s.as_str())
    }
}

impl From<&str> for CardClass {
    fn from(s: &str) -> CardClass {
        match s.to_lowercase().as_str() {
            "mega" => CardClass::Mega,
            "giga" => CardClass::Giga,
            "dark" => CardClass::Dark,
            "recipe" => CardClass::Recipe,
            _ => CardClass::Standard,
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for CardClass {
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
                    to: "CardClass",
                    message: None,
                })
            }
        };

        CardClass::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "CardClass",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for CardClass {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Number(self as u8 as f64))
    }
}
