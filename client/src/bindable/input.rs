use num_derive::FromPrimitive;
use std::hash::Hash;
use strum::EnumIter;

#[repr(u8)]
#[derive(Clone, Copy, Hash, PartialEq, Eq, Debug, EnumIter, FromPrimitive)]
pub enum Input {
    Up,
    Down,
    Left,
    Right,
    Shoot,
    UseCard,
    Special,
    Pause,
    Confirm,
    Cancel,
    Option,
    Sprint,
    ShoulderL,
    ShoulderR,
    Minimap,
    AdvanceFrame,
    RewindFrame,
}

impl<'lua> rollback_mlua::FromLua<'lua> for Input {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        use num_traits::FromPrimitive;

        let number = match lua_value {
            rollback_mlua::Value::Integer(number) => number,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "Input",
                    message: None,
                })
            }
        };

        Input::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "Input",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for Input {
    fn to_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Integer(self as i64))
    }
}
