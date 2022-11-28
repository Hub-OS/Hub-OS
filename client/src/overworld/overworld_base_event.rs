pub enum OverworldBaseEvent {
    /// See warp_system.rs to see which object types are already handled
    PendingWarp { entity: hecs::Entity },
}
