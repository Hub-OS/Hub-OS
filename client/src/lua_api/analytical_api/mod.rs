use crate::packages::Package;
use std::cell::RefCell;

mod package_management_api;

pub fn inject_analytical_api<'lua: 'scope, 'scope: 'closure, 'closure>(
    lua: &'lua rollback_mlua::Lua,
    scope: &'closure rollback_mlua::Scope<'lua, 'scope>,
    package: &'lua RefCell<impl Package + 'static>,
) -> rollback_mlua::Result<()> {
    package_management_api::inject_package_management_api(lua, scope, package)
}

pub use package_management_api::query_dependencies;
