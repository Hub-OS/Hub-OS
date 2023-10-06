use num_derive::FromPrimitive;
use serde::{Deserialize, Serialize};

#[repr(u8)]
#[derive(Default, Debug, Clone, Copy, PartialEq, Eq, FromPrimitive, Deserialize, Serialize)]
pub enum Direction {
    #[default]
    None = 0,

    // cardinal
    Up = 0b0010,
    Left = 0b1000,
    Down = 0b0001,
    Right = 0b0100,

    // diagonal
    UpLeft = 0b1010,
    UpRight = 0b0110,
    DownLeft = 0b1001,
    DownRight = 0b0101,
}

impl Direction {
    pub fn is_none(self) -> bool {
        self == Self::None
    }

    pub fn is_diagonal(self) -> bool {
        matches!(
            self,
            Direction::UpLeft | Direction::UpRight | Direction::DownLeft | Direction::DownRight
        )
    }

    /// Reverses the direction input
    pub fn reversed(self) -> Direction {
        match self {
            Direction::Up => Direction::Down,
            Direction::Left => Direction::Right,
            Direction::Down => Direction::Up,
            Direction::Right => Direction::Left,
            Direction::UpLeft => Direction::DownRight,
            Direction::UpRight => Direction::DownLeft,
            Direction::DownLeft => Direction::UpRight,
            Direction::DownRight => Direction::UpLeft,
            Direction::None => Direction::None,
        }
    }

    /// Flips the direction horizontally
    pub fn horizontal_mirror(self) -> Direction {
        match self {
            Direction::Left => Direction::Right,
            Direction::Right => Direction::Left,
            Direction::UpLeft => Direction::UpRight,
            Direction::UpRight => Direction::UpLeft,
            Direction::DownLeft => Direction::DownRight,
            Direction::DownRight => Direction::DownLeft,
            // no change
            _ => self,
        }
    }

    /// Flips the direction vertically
    pub fn vertical_mirror(self) -> Direction {
        match self {
            Direction::Up => Direction::Down,
            Direction::Down => Direction::Up,
            Direction::UpLeft => Direction::DownLeft,
            Direction::UpRight => Direction::DownRight,
            Direction::DownLeft => Direction::UpLeft,
            Direction::DownRight => Direction::UpRight,
            // no change
            _ => self,
        }
    }

    /// Splits the direction into a horizontal and vertical component
    pub fn split(self) -> (Direction, Direction) {
        match self {
            Direction::UpLeft => (Direction::Left, Direction::Up),
            Direction::UpRight => (Direction::Right, Direction::Up),
            Direction::DownLeft => (Direction::Left, Direction::Down),
            Direction::DownRight => (Direction::Right, Direction::Down),
            Direction::Left | Direction::Right => (self, Direction::None),
            Direction::Up | Direction::Down => (Direction::None, self),
            Direction::None => (Direction::None, Direction::None),
        }
    }

    pub fn join(self, b: Direction) -> Direction {
        let (ax, ay) = self.split();
        let (bx, by) = b.split();

        let horizontal = match (ax, bx) {
            (Direction::Left, Direction::Left)
            | (Direction::Left, Direction::None)
            | (Direction::None, Direction::Left) => Direction::Left,

            (Direction::Right, Direction::Right)
            | (Direction::Right, Direction::None)
            | (Direction::None, Direction::Right) => Direction::Right,

            _ => Direction::None,
        };

        let vertical = match (ay, by) {
            (Direction::Up, Direction::Up)
            | (Direction::Up, Direction::None)
            | (Direction::None, Direction::Up) => Direction::Up,

            (Direction::Down, Direction::Down)
            | (Direction::Down, Direction::None)
            | (Direction::None, Direction::Down) => Direction::Down,

            _ => Direction::None,
        };

        unsafe {
            num_traits::FromPrimitive::from_u8(horizontal as u8 | vertical as u8).unwrap_unchecked()
        }
    }

    pub fn unit_vector(self) -> (f32, f32) {
        let deg45radians = (std::f32::consts::PI / 4.0).sin();

        match self {
            Direction::Left => (-1.0, 0.0),
            Direction::Right => (1.0, 0.0),
            Direction::Up => (0.0, -1.0),
            Direction::UpLeft => (-deg45radians, -deg45radians),
            Direction::UpRight => (deg45radians, -deg45radians),
            Direction::Down => (0.0, 1.0),
            Direction::DownLeft => (-deg45radians, deg45radians),
            Direction::DownRight => (deg45radians, deg45radians),
            _ => (0.0, 0.0),
        }
    }

    /// Not sure if there's a name for this, but it's inspired by chebyshev distance.
    /// Diagonals will have both axes set to (+/-)1.0, as the chebyshev distance for (1.0, 1.0) is 1.0
    pub fn chebyshev_vector(self) -> (f32, f32) {
        match self {
            Direction::Left => (-1.0, 0.0),
            Direction::Right => (1.0, 0.0),
            Direction::Up => (0.0, -1.0),
            Direction::UpLeft => (-1.0, -1.0),
            Direction::UpRight => (1.0, -1.0),
            Direction::Down => (0.0, 1.0),
            Direction::DownLeft => (-1.0, 1.0),
            Direction::DownRight => (1.0, 1.0),
            _ => (0.0, 0.0),
        }
    }

    pub fn i32_vector(self) -> (i32, i32) {
        match self {
            Direction::Left => (-1, 0),
            Direction::Right => (1, 0),
            Direction::Up => (0, -1),
            Direction::UpLeft => (-1, -1),
            Direction::UpRight => (1, -1),
            Direction::Down => (0, 1),
            Direction::DownLeft => (-1, 1),
            Direction::DownRight => (1, 1),
            _ => (0, 0),
        }
    }

    /// Rotate clockwise
    pub fn rotate_c(self) -> Direction {
        match self {
            Direction::Up => Direction::UpRight,
            Direction::UpRight => Direction::Right,
            Direction::Right => Direction::DownRight,
            Direction::DownRight => Direction::Down,
            Direction::Down => Direction::DownLeft,
            Direction::DownLeft => Direction::Left,
            Direction::Left => Direction::UpLeft,
            Direction::UpLeft => Direction::Up,
            Direction::None => Direction::None,
        }
    }

    /// Rotate counter clockwise
    pub fn rotate_cc(self) -> Direction {
        match self {
            Direction::Up => Direction::UpLeft,
            Direction::UpRight => Direction::Up,
            Direction::Right => Direction::UpRight,
            Direction::DownRight => Direction::Right,
            Direction::Down => Direction::DownRight,
            Direction::DownLeft => Direction::Down,
            Direction::Left => Direction::DownLeft,
            Direction::UpLeft => Direction::Left,
            Direction::None => Direction::None,
        }
    }

    pub fn from_offset((x, y): (f32, f32)) -> Direction {
        if x == 0.0 && y == 0.0 {
            return Direction::None;
        }

        let x_direction = if x < 0.0 {
            Direction::UpLeft
        } else {
            Direction::DownRight
        };

        let y_direction = if y < 0.0 {
            Direction::UpRight
        } else {
            Direction::DownLeft
        };

        // using slope to calculate direction, graph if you want to take a look
        let ratio = y.abs() / x.abs();

        if ratio < 1.0 * 0.5 {
            return x_direction;
        } else if ratio > 2.0 {
            return y_direction;
        }

        x_direction.join(y_direction)
    }

    pub fn from_i32_vector((x, y): (i32, i32)) -> Direction {
        use std::cmp::Ordering;

        match (x.cmp(&0), y.cmp(&0)) {
            (Ordering::Less, Ordering::Less) => Direction::UpLeft,
            (Ordering::Less, Ordering::Equal) => Direction::Left,
            (Ordering::Less, Ordering::Greater) => Direction::DownLeft,
            (Ordering::Equal, Ordering::Less) => Direction::Up,
            (Ordering::Equal, Ordering::Equal) => Direction::None,
            (Ordering::Equal, Ordering::Greater) => Direction::Down,
            (Ordering::Greater, Ordering::Less) => Direction::UpRight,
            (Ordering::Greater, Ordering::Equal) => Direction::Right,
            (Ordering::Greater, Ordering::Greater) => Direction::DownRight,
        }
    }
}

impl From<&String> for Direction {
    fn from(s: &String) -> Direction {
        Self::from(s.as_str())
    }
}

impl From<&str> for Direction {
    fn from(direction_str: &str) -> Direction {
        let direction_string = direction_str.to_lowercase();

        match direction_string.as_str() {
            "left" => Direction::Left,
            "right" => Direction::Right,
            "up" => Direction::Up,
            "down" => Direction::Down,
            "up left" => Direction::UpLeft,
            "up right" => Direction::UpRight,
            "down left" => Direction::DownLeft,
            "down right" => Direction::DownRight,
            _ => Direction::None,
        }
    }
}

impl From<Direction> for &'static str {
    fn from(direction: Direction) -> &'static str {
        match direction {
            Direction::Left => "Left",
            Direction::Right => "Right",
            Direction::Up => "Up",
            Direction::Down => "Down",
            Direction::UpLeft => "Up Left",
            Direction::UpRight => "Up Right",
            Direction::DownLeft => "Down Left",
            Direction::DownRight => "Down Right",
            Direction::None => "",
        }
    }
}

#[cfg(feature = "rollback_mlua")]
impl<'lua> rollback_mlua::FromLua<'lua> for Direction {
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
                    to: "Direction",
                    message: None,
                })
            }
        };

        Direction::from_u8(number as u8).ok_or(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "Direction",
            message: None,
        })
    }
}

#[cfg(feature = "rollback_mlua")]
impl<'lua> rollback_mlua::IntoLua<'lua> for Direction {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Number(self as u8 as f64))
    }
}
