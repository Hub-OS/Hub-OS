use crate::bindable::Emotion;
use crate::packages::PackageNamespace;
use crate::resources::Globals;
use crate::saves::BlockGrid;
use framework::prelude::{GameIO, Vec3};
use packets::structures::PackageId;

pub struct Item {
    pub id: String,
    pub name: String,
    pub description: String,
}

pub struct OverworldPlayerData {
    pub entity: hecs::Entity,
    pub package_id: PackageId,
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
    pub fn new(game_io: &GameIO, player_entity: hecs::Entity, package_id: PackageId) -> Self {
        let mut data = Self {
            entity: player_entity,
            package_id,
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

        data.process_boosts(game_io);
        data.health = data.base_health + data.health_boost;

        data
    }

    pub fn max_health(&self) -> i32 {
        self.base_health + self.health_boost
    }

    pub fn process_boosts(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        let global_save = &globals.global_save;

        // base_health
        let player_package = global_save.player_package(game_io).unwrap();
        self.base_health = player_package.health;

        // health_boost
        let blocks = global_save.active_blocks().cloned().unwrap_or_default();
        let block_grid = BlockGrid::new(PackageNamespace::Server).with_blocks(game_io, blocks);

        let mut health_boost = 0;

        for (package, level) in block_grid.augments(game_io) {
            health_boost += package.health_boost * level as i32;
        }

        self.health_boost = health_boost;

        // apply max_health to final health
        self.health = self.max_health().min(self.health);
    }
}
