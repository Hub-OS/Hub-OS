use super::*;
use crate::battle::{BattleSimulation, Entity};
use crate::bindable::Element;
use crate::lua_api::{create_analytical_vm, create_battle_vm};
use crate::render::{Animator, Background};
use crate::resources::{AssetManager, Globals, LocalAssetManager, ResourcePaths};
use framework::prelude::{GameIO, Texture};
use std::cell::RefCell;
use std::sync::Arc;

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
    pub fn resolve_battle_sprite(&self, game_io: &GameIO<Globals>) -> (Arc<Texture>, Animator) {
        let globals = game_io.globals();
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

    fn load_new(assets: &LocalAssetManager, package_info: PackageInfo) -> Self {
        let package = RefCell::new(PlayerPackage::default());
        package.borrow_mut().package_info = package_info.clone();

        let lua = create_analytical_vm(assets, &package_info);

        let globals = lua.globals();
        let package_init: rollback_mlua::Function = match globals.get("package_init") {
            Ok(package_init) => package_init,
            _ => {
                log::error!(
                    "missing package_init() in {:?}",
                    ResourcePaths::shorten(&package_info.script_path)
                );
                return package.into_inner();
            }
        };

        let result = lua.scope(|scope| {
            crate::lua_api::analytical_api::inject_analytical_api(&lua, scope, assets, &package)?;
            crate::lua_api::analytical_api::query_dependencies(&lua);

            let package_table = lua.create_table()?;

            package_table.set(
                "declare_package_id",
                scope.create_function(|_, (_, id): (rollback_mlua::Table, String)| {
                    package.borrow_mut().package_info.id = id;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_name",
                scope.create_function(|_, (_, name): (rollback_mlua::Table, String)| {
                    package.borrow_mut().name = name;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_health",
                scope.create_function(|_, (_, health): (rollback_mlua::Table, i32)| {
                    package.borrow_mut().health = health;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_element",
                scope.create_function(|_, (_, element): (rollback_mlua::Table, Element)| {
                    package.borrow_mut().element = element;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_description",
                scope.create_function(|_, (_, description): (rollback_mlua::Table, String)| {
                    package.borrow_mut().description = description;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_icon_texture_path",
                scope.create_function(|_, (_, path): (rollback_mlua::Table, String)| {
                    package.borrow_mut().icon_texture_path =
                        package_info.base_path.to_string() + &path;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_preview_texture_path",
                scope.create_function(|_, (_, path): (rollback_mlua::Table, String)| {
                    package.borrow_mut().preview_texture_path =
                        package_info.base_path.to_string() + &path;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_overworld_texture_path",
                scope.create_function(|_, (_, path): (rollback_mlua::Table, String)| {
                    package.borrow_mut().overworld_texture_path =
                        package_info.base_path.to_string() + &path;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_overworld_animation_path",
                scope.create_function(|_, (_, path): (rollback_mlua::Table, String)| {
                    package.borrow_mut().overworld_animation_path =
                        package_info.base_path.to_string() + &path;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_mugshot_texture_path",
                scope.create_function(|_, (_, path): (rollback_mlua::Table, String)| {
                    package.borrow_mut().mugshot_texture_path =
                        package_info.base_path.to_string() + &path;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_mugshot_animation_path",
                scope.create_function(|_, (_, path): (rollback_mlua::Table, String)| {
                    package.borrow_mut().mugshot_animation_path =
                        package_info.base_path.to_string() + &path;
                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_emotions_texture_path",
                scope.create_function(|_, (_, path): (rollback_mlua::Table, String)| {
                    package.borrow_mut().emotions_texture_path =
                        package_info.base_path.to_string() + &path;
                    Ok(())
                })?,
            )?;

            package_init.call(package_table)?;

            Ok(())
        });

        if let Err(e) = result {
            log::error!("{e}");
        }

        package.into_inner()
    }
}
