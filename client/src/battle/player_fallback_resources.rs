use crate::battle::{
    BattleProps, BattleSimulation, Entity, Player, PlayerSetup, SharedBattleResources,
};
use crate::packages::{PackageNamespace, PlayerPackage};
use crate::render::Animator;
use crate::resources::Globals;
use framework::prelude::GameIO;
use packets::structures::PackageCategory;

#[derive(Default)]
pub struct PlayerFallbackResources {
    pub texture_path: String,
    pub animator: Animator,
    pub palette_path: Option<String>,
}

impl PlayerFallbackResources {
    pub fn resolve(game_io: &GameIO, package: &PlayerPackage) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let package_info = &package.package_info;

        // create simulation
        let setup = PlayerSetup::new_dummy(package, 0, true);
        let mut props = BattleProps {
            player_setups: vec![setup],
            ..BattleProps::new_with_defaults(game_io, None)
        };
        let mut simulation = BattleSimulation::new(game_io, &props);

        // load vms
        let inital_iter = std::iter::once((
            PackageCategory::Player,
            PackageNamespace::Netplay(0),
            package_info.id.clone(),
        ));

        let dependencies = globals.package_dependency_iter(inital_iter);
        let resources = SharedBattleResources::new(game_io, &mut simulation, &dependencies);

        // load player into the simulation
        let setup = props.player_setups.pop().unwrap();
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
