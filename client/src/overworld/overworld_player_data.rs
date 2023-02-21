use crate::bindable::Emotion;
use crate::packages::PackageNamespace;
use crate::resources::Globals;
use crate::saves::BlockGrid;
use framework::prelude::{GameIO, Vec3};

pub struct Item {
    pub id: String,
    pub name: String,
    pub description: String,
}

pub struct OverworldPlayerData {
    pub entity: hecs::Entity,
    pub health: i32,
    pub base_health: i32,
    pub health_boost: i32,
    pub emotion: Emotion,
    pub money: u32,
    pub items: Vec<Item>,
    pub actor_interaction: Option<String>,
    pub object_interaction: Option<u32>,
    pub tile_interaction: Option<Vec3>,
}

impl OverworldPlayerData {
    pub fn new(game_io: &GameIO, player_entity: hecs::Entity) -> Self {
        let mut data = Self {
            entity: player_entity,
            health: 0,
            base_health: 0,
            health_boost: 0,
            emotion: Emotion::Normal,
            money: 0,
            items: Vec::new(),
            actor_interaction: None,
            object_interaction: None,
            tile_interaction: None,
        };

        data.process_augments(game_io);
        data.health = data.base_health + data.health_boost;

        data
    }

    pub fn max_health(&self) -> i32 {
        self.base_health + self.health_boost
    }

    pub fn process_augments(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        let global_save = &globals.global_save;
        let blocks = global_save.active_blocks().cloned().unwrap_or_default();
        let block_grid = BlockGrid::new(PackageNamespace::Server).with_blocks(game_io, blocks);

        let mut health_boost = 0;

        for package in block_grid.valid_packages(game_io) {
            health_boost += package.health_boost;
        }

        self.health_boost = health_boost;

        let global_save = &globals.global_save;
        let player_package = global_save.player_package(game_io).unwrap();
        self.base_health = player_package.health;
    }
}
