pub struct PlayerData {
    pub identity: Vec<u8>,
    pub avatar_name: String,
    pub element: String,
    pub health: i32,
    pub base_health: i32,
    pub health_boost: i32,
    pub emotion: u8,
    pub money: u32,
    pub items: Vec<String>,
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
            emotion: 0,
            money: 0,
            items: Vec::new(),
        }
    }

    pub fn max_health(&self) -> i32 {
        self.base_health + self.health_boost
    }
}
