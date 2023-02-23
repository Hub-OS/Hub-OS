use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Deserialize, Serialize)]
#[repr(u8)]
pub enum PackageCategory {
    Augment,
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
            PackageCategory::Augment => "mods/augments/",
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
            "augment" => Self::Augment,
            "card" => Self::Card,
            "battle" => Self::Battle,
            "character" => Self::Character,
            "player" => Self::Player,
            _ => Self::Library,
        }
    }
}
