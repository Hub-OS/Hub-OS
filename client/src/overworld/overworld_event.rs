use crate::render::ui::PackageListing;
use framework::prelude::{NextScene, Vec3};
use packets::structures::{BattleStatistics, Direction};

pub enum OverworldEvent {
    SystemMessage {
        message: String,
    },
    TextboxResponse(u8),
    PromptResponse(String),
    BattleStatistics(Option<BattleStatistics>),
    WarpIn {
        target_entity: hecs::Entity,
        position: Vec3,
        direction: Direction,
    },
    /// See warp_system.rs to see which object types are already handled
    PendingWarp {
        entity: hecs::Entity,
    },
    TransferServer {
        address: String,
        data: Option<String>,
    },
    Disconnected {
        message: String,
    },
    PackageReferred(PackageListing),
    NextScene(NextScene),
    Leave,
}
