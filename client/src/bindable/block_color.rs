use num_derive::FromPrimitive;
use serde::{Deserialize, Serialize};

#[repr(u8)]
#[derive(Default, Clone, Copy, PartialEq, Eq, FromPrimitive, Serialize, Deserialize)]
pub enum BlockColor {
    #[default]
    White,
    Red,
    Green,
    Blue,
    Pink,
    Yellow,
}

impl BlockColor {
    pub fn flat_state(&self) -> &'static str {
        match self {
            BlockColor::White => "WHITE_FLAT",
            BlockColor::Red => "RED_FLAT",
            BlockColor::Green => "GREEN_FLAT",
            BlockColor::Blue => "BLUE_FLAT",
            BlockColor::Pink => "PINK_FLAT",
            BlockColor::Yellow => "YELLOW_FLAT",
        }
    }

    pub fn plus_state(&self) -> &'static str {
        match self {
            BlockColor::White => "WHITE_PLUS",
            BlockColor::Red => "RED_PLUS",
            BlockColor::Green => "GREEN_PLUS",
            BlockColor::Blue => "BLUE_PLUS",
            BlockColor::Pink => "PINK_PLUS",
            BlockColor::Yellow => "YELLOW_PLUS",
        }
    }

    pub fn flat_held_state(&self) -> &'static str {
        match self {
            BlockColor::White => "WHITE_FLAT_HELD",
            BlockColor::Red => "RED_FLAT_HELD",
            BlockColor::Green => "GREEN_FLAT_HELD",
            BlockColor::Blue => "BLUE_FLAT_HELD",
            BlockColor::Pink => "PINK_FLAT_HELD",
            BlockColor::Yellow => "YELLOW_FLAT_HELD",
        }
    }

    pub fn plus_held_state(&self) -> &'static str {
        match self {
            BlockColor::White => "WHITE_PLUS_HELD",
            BlockColor::Red => "RED_PLUS_HELD",
            BlockColor::Green => "GREEN_PLUS_HELD",
            BlockColor::Blue => "BLUE_PLUS_HELD",
            BlockColor::Pink => "PINK_PLUS_HELD",
            BlockColor::Yellow => "YELLOW_PLUS_HELD",
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for BlockColor {
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
                    to: "BlockColor",
                    message: None,
                })
            }
        };

        BlockColor::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "BlockColor",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for BlockColor {
    fn to_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Number(self as u8 as f64))
    }
}
