use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Deserialize, Serialize)]
#[repr(u8)]
pub enum PackageCategory {
    Augment,
    Battle,
    Card,
    Character,
    #[default]
    Library,
    Player,
}

impl PackageCategory {
    pub fn path(&self) -> &'static str {
        match self {
            PackageCategory::Augment => "mods/augments/",
            PackageCategory::Character | PackageCategory::Battle => "mods/battles/",
            PackageCategory::Card => "mods/cards/",
            PackageCategory::Library => "mods/libraries/",
            PackageCategory::Player => "mods/players/",
        }
    }
}

impl From<&str> for PackageCategory {
    fn from(s: &str) -> Self {
        match s.to_lowercase().as_str() {
            "augment" => Self::Augment,
            "battle" => Self::Battle,
            "card" => Self::Card,
            "character" => Self::Character,
            "player" => Self::Player,
            _ => Self::Library,
        }
    }
}
