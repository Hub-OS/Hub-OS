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
    Leave,
}
