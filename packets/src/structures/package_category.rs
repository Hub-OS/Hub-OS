use serde::{Deserialize, Serialize};
use strum::EnumIter;

#[derive(Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Deserialize, Serialize, EnumIter)]
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
    TileState,
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
            PackageCategory::TileState => "mods/tile_states/",
        }
    }

    pub fn built_in_path(&self) -> &'static str {
        match self {
            PackageCategory::Augment => "resources/packages/augments/",
            PackageCategory::Character | PackageCategory::Encounter => {
                "resources/packages/encounters/"
            }
            PackageCategory::Card => "resources/packages/cards/",
            PackageCategory::Library => "resources/packages/libraries/",
            PackageCategory::Player => "resources/packages/players/",
            PackageCategory::Resource => "resources/packages/resources/",
            PackageCategory::Status => "resources/packages/statuses/",
            PackageCategory::TileState => "resources/packages/tile_states/",
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
            "status" => Self::Status,
            "tile_state" => Self::TileState,
            _ => Self::Library,
        }
    }
}
