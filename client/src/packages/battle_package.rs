use super::*;
use crate::lua_api::create_analytical_vm;
use crate::resources::{AssetManager, ResourcePaths};
use std::cell::RefCell;

#[derive(Default, Clone)]
pub struct BattlePackage {
    pub package_info: PackageInfo,
    pub name: String,
    pub description: String,
    pub preview_texture_path: String,
}

impl Package for BattlePackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
    }

    fn load_new(assets: &impl AssetManager, package_info: PackageInfo) -> Self {
        let package = RefCell::new(BattlePackage::default());
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

        lua.scope(|scope| {
            crate::lua_api::inject_analytical_api(&lua, scope, &package)?;
            crate::lua_api::query_dependencies(&lua);

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
                "set_description",
                scope.create_function(|_, (_, description): (rollback_mlua::Table, String)| {
                    package.borrow_mut().description = description;
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

            match package_init.call(package_table) {
                Ok(()) => {}
                Err(e) => {
                    log::error!("{}", e);
                }
            };

            Ok(())
        })
        .unwrap();

        package.into_inner()
    }
}
