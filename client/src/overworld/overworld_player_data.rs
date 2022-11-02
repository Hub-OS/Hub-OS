use crate::bindable::Emotion;
use framework::prelude::Vec3;

pub struct Item {
    pub id: String,
    pub name: String,
    pub description: String,
}

pub struct OverworldPlayerData {
    pub entity: hecs::Entity,
    pub health: i32,
    pub max_health: i32,
    pub emotion: Emotion,
    pub money: u32,
    pub items: Vec<Item>,
    pub actor_interaction: Option<String>,
    pub object_interaction: Option<u32>,
    pub tile_interaction: Option<Vec3>,
}

impl OverworldPlayerData {
    pub fn new(player_entity: hecs::Entity) -> Self {
        Self {
            entity: player_entity,
            health: 100,
            max_health: 100,
            emotion: Emotion::Normal,
            money: 0,
            items: Vec::new(),
            actor_interaction: None,
            object_interaction: None,
            tile_interaction: None,
        }
    }
}
