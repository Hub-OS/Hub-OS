use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Deserialize, Serialize)]
#[repr(u8)]
pub enum PackageCategory {
    Block,
    Card,
    Battle,
    Character,
    #[default]
    Library,
    Player,
}

impl PackageCategory {
    pub fn path(&self) -> &'static str {
        match self {
            PackageCategory::Block => "mods/blocks/",
            PackageCategory::Card => "mods/cards/",
            PackageCategory::Character | PackageCategory::Battle => "mods/enemies/",
            PackageCategory::Library => "mods/libraries/",
            PackageCategory::Player => "mods/players/",
        }
    }
}

impl From<&str> for PackageCategory {
    fn from(s: &str) -> Self {
        match s.to_lowercase().as_str() {
            "block" => Self::Block,
            "card" => Self::Card,
            "battle" => Self::Battle,
            "character" => Self::Character,
            "player" => Self::Player,
            _ => Self::Library,
        }
    }
}
