use num_derive::FromPrimitive;
use serde::{Deserialize, Serialize};

#[repr(u8)]
#[derive(
    Default,
    Debug,
    Clone,
    Copy,
    Hash,
    PartialEq,
    Eq,
    PartialOrd,
    Ord,
    FromPrimitive,
    Serialize,
    Deserialize,
)]
pub enum BlockColor {
    #[default]
    White,
    Red,
    Green,
    Blue,
    Pink,
    Yellow,
    Orange,
}

impl BlockColor {
    pub const MAX_TEXT_WIDTH: usize = 6;

    pub fn state(&self) -> &'static str {
        match self {
            BlockColor::White => "WHITE_INDICATOR",
            BlockColor::Red => "RED_INDICATOR",
            BlockColor::Green => "GREEN_INDICATOR",
            BlockColor::Blue => "BLUE_INDICATOR",
            BlockColor::Pink => "PINK_INDICATOR",
            BlockColor::Yellow => "YELLOW_INDICATOR",
            BlockColor::Orange => "ORANGE_INDICATOR",
        }
    }

    pub fn flat_state(&self) -> &'static str {
        match self {
            BlockColor::White => "WHITE_FLAT",
            BlockColor::Red => "RED_FLAT",
            BlockColor::Green => "GREEN_FLAT",
            BlockColor::Blue => "BLUE_FLAT",
            BlockColor::Pink => "PINK_FLAT",
            BlockColor::Yellow => "YELLOW_FLAT",
            BlockColor::Orange => "ORANGE_FLAT",
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
            BlockColor::Orange => "ORANGE_PLUS",
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
            BlockColor::Orange => "ORANGE_FLAT_HELD",
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
            BlockColor::Orange => "ORANGE_PLUS_HELD",
        }
    }
}

impl From<String> for BlockColor {
    fn from(text: String) -> Self {
        text.as_str().into()
    }
}

impl From<&str> for BlockColor {
    fn from(text: &str) -> Self {
        match text.to_lowercase().as_str() {
            "red" => BlockColor::Red,
            "green" => BlockColor::Green,
            "blue" => BlockColor::Blue,
            "pink" => BlockColor::Pink,
            "yellow" => BlockColor::Yellow,
            "orange" => BlockColor::Orange,
            _ => BlockColor::White,
        }
    }
}
