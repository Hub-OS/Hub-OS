use super::*;
use crate::battle::{BattleSimulation, Entity};
use crate::bindable::Element;
use crate::lua_api::create_battle_vm;
use crate::render::ui::{PackageListing, PackagePreviewData};
use crate::render::{Animator, Background};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::{GameIO, Texture};
use serde::Deserialize;
use std::sync::Arc;

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
}

impl PlayerPackage {
    pub fn resolve_battle_sprite(&self, game_io: &GameIO) -> (Arc<Texture>, Animator) {
        let globals = game_io.resource::<Globals>().unwrap();
        let package_info = &self.package_info;

        // create simulation
        let background = Background::new_blank(game_io);
        let mut simulation = BattleSimulation::new(game_io, background, 1);

        // load vms
        let mut vms = Vec::new();

        let inital_iter = std::iter::once((
            PackageCategory::Player,
            package_info.namespace,
            package_info.id.clone(),
        ));

        for package_info in globals.package_dependency_iter(inital_iter) {
            create_battle_vm(game_io, &mut simulation, &mut vms, package_info);
        }

        // load player into the simulation
        let Ok(entity_id) = simulation.load_player(
            game_io,
            &vms,
            &self.package_info.id,
            self.package_info.namespace,
            0,
            true,
            vec![],
            vec![],
        ) else {
            return (globals.assets.texture(game_io, ResourcePaths::BLANK), Animator::new())
        };

        // grab the entitiy
        let entity = simulation
            .entities
            .query_one_mut::<&Entity>(entity_id.into())
            .unwrap();

        // clone the texture
        let sprite_node = entity.sprite_tree.root();
        let texture = sprite_node.texture().clone();

        // clone the animator
        let battle_animator = &simulation.animators[entity.animator_index];
        let animator = battle_animator.animator().clone();

        (texture, animator)
    }
}

impl Package for PlayerPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
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
                log::error!("failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "player" {
            log::error!(
                "missing `category = \"player\"` in {:?}",
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

        package
    }
}
