use super::*;
use crate::battle::{
    BattleProps, BattleSimulation, Entity, Player, PlayerSetup, SharedBattleResources,
};
use crate::bindable::Element;
use crate::render::ui::{PackageListing, PackagePreviewData};
use crate::render::Animator;
use crate::resources::{Globals, ResourcePaths};
use framework::prelude::GameIO;
use serde::Deserialize;

#[derive(Deserialize, Default)]
#[serde(default)]
struct PlayerMeta {
    category: String,
    name: String,
    health: i32,
    element: String,
    description: String,
    preview_texture_path: String,
    overworld_animation_path: String,
    overworld_texture_path: String,
    mugshot_texture_path: String,
    mugshot_animation_path: String,
    emotions_texture_path: String,
    emotions_animation_path: String,
}

#[derive(Default, Clone)]
pub struct PlayerPackage {
    pub package_info: PackageInfo,
    pub name: String,
    pub health: i32,
    pub element: Element,
    pub description: String,
    pub icon_texture_path: String,
    pub preview_texture_path: String,
    pub overworld_animation_path: String,
    pub overworld_texture_path: String,
    pub mugshot_texture_path: String,
    pub mugshot_animation_path: String,
    pub emotions_texture_path: String,
    pub emotions_animation_path: String,
}

impl PlayerPackage {
    /// Returns (texture_path, animator)
    pub fn resolve_battle_sprite(&self, game_io: &GameIO) -> (String, Animator) {
        let globals = game_io.resource::<Globals>().unwrap();
        let package_info = &self.package_info;

        // create simulation
        let setup = PlayerSetup::new(self, 0, true);
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
        let Ok(entity_id) = Player::load(game_io, &resources, &mut simulation, setup) else {
            return (ResourcePaths::BLANK.to_string(), Animator::new());
        };

        // grab the entitiy
        let entity = simulation
            .entities
            .query_one_mut::<&Entity>(entity_id.into())
            .unwrap();

        // clone the texture path
        let texture_path = simulation
            .sprite_trees
            .get(entity.sprite_tree_index)
            .map(|sprite_tree| sprite_tree.root().texture_path().to_string())
            .unwrap_or_default();

        // clone the animator
        let battle_animator = simulation.animators.remove(entity.animator_index).unwrap();
        let animator = battle_animator.take_animator();

        (texture_path, animator)
    }
}

impl Package for PlayerPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            id: self.package_info.id.clone(),
            name: self.name.clone(),
            description: self.description.clone(),
            creator: String::new(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::Player {
                element: self.element,
                health: self.health,
            },
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = PlayerPackage {
            package_info,
            ..Default::default()
        };

        let meta: PlayerMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "player" {
            log::error!(
                "Missing `category = \"player\"` in {:?}",
                package.package_info.toml_path
            );
        }

        let base_path = &package.package_info.base_path;

        package.name = meta.name;
        package.health = meta.health;
        package.description = meta.description;
        package.preview_texture_path = base_path.clone() + &meta.preview_texture_path;
        package.overworld_animation_path = base_path.clone() + &meta.overworld_animation_path;
        package.overworld_texture_path = base_path.clone() + &meta.overworld_texture_path;
        package.mugshot_texture_path = base_path.clone() + &meta.mugshot_texture_path;
        package.mugshot_animation_path = base_path.clone() + &meta.mugshot_animation_path;
        package.emotions_texture_path = base_path.clone() + &meta.emotions_texture_path;
        package.emotions_animation_path = base_path.clone() + &meta.emotions_animation_path;

        package
    }
}
