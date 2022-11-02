use super::*;
use crate::lua_api::create_analytical_vm;
use crate::resources::AssetManager;
use crate::{bindable::CardProperties, resources::ResourcePaths};
use rollback_mlua::{FromLua, ToLua};
use std::cell::RefCell;

#[derive(Default, Clone)]
pub struct CardPackage {
    pub package_info: PackageInfo,
    pub icon_texture_path: String,
    pub preview_texture_path: String,
    pub card_properties: CardProperties,
    pub default_codes: Vec<String>,
}

impl Package for CardPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
    }

    fn load_new(assets: &impl AssetManager, package_info: PackageInfo) -> Self {
        let package = RefCell::new(CardPackage::default());
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

        // create a table for card properties
        let table = CardProperties::default().to_lua(&lua).unwrap();

        lua.scope(|scope| {
            crate::lua_api::inject_analytical_api(&lua, scope, &package)?;
            crate::lua_api::query_dependencies(&lua);

            let package_table = lua.create_table()?;

            package_table.set(
                "declare_package_id",
                scope.create_function(|_, (_, id): (rollback_mlua::Table, String)| {
                    let mut package = package.borrow_mut();
                    package.package_info.id = id;

                    Ok(())
                })?,
            )?;

            package_table.set(
                "set_codes",
                scope.create_function(|_, (_, codes): (rollback_mlua::Table, Vec<String>)| {
                    package.borrow_mut().default_codes = codes;

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
                "get_card_props",
                scope.create_function(|_, _: ()| Ok(table.clone()))?,
            )?;

            if let Err(e) = package_init.call::<rollback_mlua::Table, ()>(package_table) {
                log::error!("{}", e);
            };

            let mut card_properties = CardProperties::from_lua(table.clone(), &lua)?;
            card_properties.package_id = package.borrow().package_info.id.clone();

            package.borrow_mut().card_properties = card_properties;
            Ok(())
        })
        .unwrap();

        package.into_inner()
    }
}
