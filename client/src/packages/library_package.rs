use super::*;
use crate::lua_api::create_analytical_vm;
use crate::resources::AssetManager;
use std::cell::RefCell;

#[derive(Default, Clone)]
pub struct LibraryPackage {
    pub package_info: PackageInfo,
}

impl Package for LibraryPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
    }

    fn load_new(assets: &impl AssetManager, package_info: PackageInfo) -> Self {
        let lua = create_analytical_vm(assets, &package_info);

        let package = RefCell::new(Self { package_info });

        lua.scope(|scope| {
            crate::lua_api::inject_analytical_api(&lua, scope, &package)?;
            crate::lua_api::query_dependencies(&lua);

            Ok(())
        })
        .unwrap();

        package.into_inner()
    }
}
