use crate::bindable::Emotion;
use crate::packages::PackageNamespace;
use crate::resources::Globals;
use crate::saves::BlockGrid;
use framework::prelude::{GameIO, Vec3};
use packets::structures::{ActorId, Inventory, PackageId};

pub struct OverworldPlayerData {
    pub entity: hecs::Entity,
    pub package_id: PackageId,
    pub health: i32,
    pub base_health: i32,
    pub health_boost: i32,
    pub emotion: Emotion,
    pub money: u32,
    pub inventory: Inventory,
    pub actor_interaction: Option<ActorId>,
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
            emotion: Emotion::default(),
            money: 0,
            inventory: Inventory::new(),
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
        let globals = Globals::from_resources(game_io);

        let global_save = &globals.global_save;
        let restrictions = &globals.restrictions;

        // base_health
        let player_package = global_save.player_package(game_io).unwrap();
        self.base_health = player_package.health;

        // resolving health boost from augments
        let mut health_boost = 0;

        // health boost from blocks
        let ns = PackageNamespace::Local;
        let blocks: Vec<_> = restrictions
            .filter_blocks(game_io, ns, global_save.active_blocks().iter())
            .cloned()
            .collect();

        let block_grid = BlockGrid::new(ns).with_blocks(game_io, blocks);

        for (package, level) in block_grid.augments(game_io) {
            health_boost += package.health_boost * level as i32;
        }

        // health boost from switch drives
        let drives: Vec<_> = restrictions
            .filter_drives(game_io, ns, global_save.active_drive_parts().iter())
            .cloned()
            .collect();

        health_boost += drives
            .iter()
            .flat_map(|drive| globals.augment_packages.package(ns, &drive.package_id))
            .fold(0, |acc, package| acc + package.health_boost);

        self.health_boost = health_boost;

        // apply max_health to final health
        self.health = self.max_health().min(self.health);
    }
}
