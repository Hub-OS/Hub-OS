use packets::structures::{Emotion, Inventory};

pub struct PlayerData {
    pub identity: Vec<u8>,
    pub avatar_name: String,
    pub element: String,
    pub health: i32,
    pub base_health: i32,
    pub health_boost: i32,
    pub emotion: Emotion,
    pub money: u32,
    pub inventory: Inventory,
}

impl PlayerData {
    pub fn new(identity: Vec<u8>) -> PlayerData {
        PlayerData {
            identity,
            avatar_name: String::new(),
            element: String::new(),
            health: 0,
            base_health: 0,
            health_boost: 0,
            emotion: Emotion::default(),
            money: 0,
            inventory: Inventory::new(),
        }
    }

    pub fn max_health(&self) -> i32 {
        self.base_health + self.health_boost
    }
}
