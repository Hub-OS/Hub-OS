use crate::battle::{BattleSimulation, Entity, Player, PlayerSetup, SharedBattleResources};
use crate::packages::PlayerPackage;
use crate::render::Animator;
use framework::prelude::GameIO;

use super::BattleMeta;

#[derive(Default)]
pub struct PlayerFallbackResources {
    pub texture_path: String,
    pub animator: Animator,
    pub palette_path: Option<String>,
}

impl PlayerFallbackResources {
    pub fn resolve(game_io: &GameIO, package: &PlayerPackage) -> Self {
        // create simulation
        let setup = PlayerSetup {
            package_id: package.package_info.id.clone(),
            ..PlayerSetup::new_empty(0, true)
        };

        let mut meta = BattleMeta {
            player_setups: vec![setup],
            ..BattleMeta::new_with_defaults(game_io, None)
        };
        let mut simulation = BattleSimulation::new(game_io, &meta);

        // load vms
        let mut resources = SharedBattleResources::new(game_io, &meta);
        resources.load_encounter_vms(game_io, &mut simulation, &meta);
        resources.load_player_vms(game_io, &mut simulation, &meta);

        // load player into the simulation
        let setup = meta.player_setups.pop().unwrap();
        let Ok(entity_id) = Player::load(game_io, &resources, &mut simulation, &setup) else {
            return Default::default();
        };

        // grab the entitiy
        let entity = simulation
            .entities
            .query_one_mut::<&Entity>(entity_id.into())
            .unwrap();

        // clone the texture path
        let mut texture_path = String::new();
        let mut palette_path = None;

        if let Some(sprite_tree) = simulation.sprite_trees.get(entity.sprite_tree_index) {
            let sprite_node = sprite_tree.root();

            texture_path = sprite_node.texture_path().to_string();
            palette_path = sprite_node.palette_path().map(str::to_string)
        }

        // clone the animator
        let battle_animator = simulation.animators.remove(entity.animator_index).unwrap();
        let animator = battle_animator.take_animator();

        Self {
            texture_path,
            animator,
            palette_path,
        }
    }
}
