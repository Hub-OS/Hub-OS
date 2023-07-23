use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Deserialize, Serialize)]
#[repr(u8)]
pub enum PackageCategory {
    Augment,
    Encounter,
    Card,
    Character,
    #[default]
    Library,
    Player,
    Resource,
}

impl PackageCategory {
    pub fn path(&self) -> &'static str {
        match self {
            PackageCategory::Augment => "mods/augments/",
            PackageCategory::Character | PackageCategory::Encounter => "mods/encounters/",
            PackageCategory::Card => "mods/cards/",
            PackageCategory::Library => "mods/libraries/",
            PackageCategory::Player => "mods/players/",
            PackageCategory::Resource => "mods/resources/",
        }
    }

    pub fn requires_vm(self) -> bool {
        self != PackageCategory::Library
    }
}

impl From<&str> for PackageCategory {
    fn from(s: &str) -> Self {
        match s.to_lowercase().as_str() {
            "augment" => Self::Augment,
            "encounter" => Self::Encounter,
            "card" => Self::Card,
            "character" => Self::Character,
            "player" => Self::Player,
            "resource" => Self::Resource,
            _ => Self::Library,
        }
    }
}
