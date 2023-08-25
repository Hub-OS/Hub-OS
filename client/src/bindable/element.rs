use num_derive::FromPrimitive;
use strum::Display;

#[repr(u8)]
#[derive(PartialEq, Eq, Default, Clone, Copy, FromPrimitive, Display)]
pub enum Element {
    #[default]
    None,
    Fire,
    Aqua,
    Elec,
    Wood,
    Sword,
    Wind,
    Cursor,
    Summon,
    Plus,
    Break,
}

impl Element {
    pub fn is_weak_to(self, other: Element) -> bool {
        matches!(
            (self, other),
            (Element::Aqua, Element::Elec)
                | (Element::Fire, Element::Aqua)
                | (Element::Wood, Element::Fire)
                | (Element::Elec, Element::Wood)
                | (Element::Sword, Element::Break)
                | (Element::Wind, Element::Sword)
                | (Element::Cursor, Element::Wind)
                | (Element::Break, Element::Cursor)
        )
    }
}

impl From<String> for Element {
    fn from(s: String) -> Self {
        Self::from(s.as_str())
    }
}

impl From<&str> for Element {
    fn from(s: &str) -> Element {
        match s.to_lowercase().as_str() {
            "fire" => Element::Fire,
            "aqua" => Element::Aqua,
            "elec" => Element::Elec,
            "wood" => Element::Wood,
            "sword" => Element::Sword,
            "wind" => Element::Wind,
            "cursor" => Element::Cursor,
            "summon" => Element::Summon,
            "plus" => Element::Plus,
            "break" => Element::Break,
            _ => Element::None,
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for Element {
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
                    to: "Element",
                    message: None,
                })
            }
        };

        Element::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "Element",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for Element {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Number(self as u8 as f64))
    }
}
