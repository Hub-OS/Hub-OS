use num_derive::FromPrimitive;

use super::Direction;

#[repr(u8)]
#[derive(PartialEq, Eq, Default, Clone, Copy, FromPrimitive)]
pub enum TileState {
    #[default]
    Normal = 0,
    Cracked,
    Broken,
    Ice,
    Grass,
    Lava,
    Poison,
    Empty,
    Holy,
    DirectionLeft,
    DirectionRight,
    DirectionUp,
    DirectionDown,
    Volcano,
    Sea,
    Sand,
    Metal,
    Hidden, // immutable
}

impl TileState {
    pub fn is_walkable(self) -> bool {
        !self.is_hole()
    }

    pub fn is_hole(self) -> bool {
        matches!(
            self,
            TileState::Hidden | TileState::Broken | TileState::Empty
        )
    }

    pub fn immutable(self) -> bool {
        matches!(self, TileState::Hidden)
    }

    pub fn direction(self) -> Direction {
        match self {
            TileState::DirectionLeft => Direction::Left,
            TileState::DirectionRight => Direction::Right,
            TileState::DirectionUp => Direction::Up,
            TileState::DirectionDown => Direction::Down,
            _ => Direction::None,
        }
    }

    pub fn animation_suffix(self, flipped: bool) -> &'static str {
        match self {
            TileState::Normal => "normal",
            TileState::Cracked => "cracked",
            TileState::Broken => "broken",
            TileState::Ice => "ice",
            TileState::Grass => "grass",
            TileState::Lava => "lava",
            TileState::Poison => "poison",
            TileState::Empty => "empty",
            TileState::Holy => "holy",
            TileState::DirectionLeft => {
                if flipped {
                    "direction_right"
                } else {
                    "direction_left"
                }
            }
            TileState::DirectionRight => {
                if flipped {
                    "direction_left"
                } else {
                    "direction_right"
                }
            }
            TileState::DirectionUp => "direction_up",
            TileState::DirectionDown => "direction_down",
            TileState::Volcano => "volcano",
            TileState::Sea => "sea",
            TileState::Sand => "sand",
            TileState::Metal => "metal",
            TileState::Hidden => "",
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for TileState {
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
                    to: "TileState",
                    message: None,
                })
            }
        };

        TileState::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "TileState",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for TileState {
    fn to_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Number(self as u8 as f64))
    }
}
