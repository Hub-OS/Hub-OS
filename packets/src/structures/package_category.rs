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
    Status,
}

impl PackageCategory {
    pub fn mod_path(&self) -> &'static str {
        match self {
            PackageCategory::Augment => "mods/augments/",
            PackageCategory::Character | PackageCategory::Encounter => "mods/encounters/",
            PackageCategory::Card => "mods/cards/",
            PackageCategory::Library => "mods/libraries/",
            PackageCategory::Player => "mods/players/",
            PackageCategory::Resource => "mods/resources/",
            PackageCategory::Status => "mods/statuses/",
        }
    }

    pub fn built_in_path(&self) -> &'static str {
        match self {
            PackageCategory::Augment => "resources/packages/augments/",
            PackageCategory::Character | PackageCategory::Encounter => "mods/encounters/",
            PackageCategory::Card => "resources/packages/cards/",
            PackageCategory::Library => "resources/packages/libraries/",
            PackageCategory::Player => "resources/packages/players/",
            PackageCategory::Resource => "resources/packages/resources/",
            PackageCategory::Status => "resources/packages/statuses/",
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
