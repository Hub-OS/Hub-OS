use crate::lua_api::helpers::absolute_path;
use crate::packages::{Package, PackageCategory};
use std::cell::RefCell;

pub fn inject_package_management_api<'lua: 'scope, 'scope: 'closure, 'closure>(
    lua: &'lua rollback_mlua::Lua,
    scope: &'closure rollback_mlua::Scope<'lua, 'scope>,
    package: &'lua RefCell<impl Package + 'static>,
) -> rollback_mlua::Result<()> {
    let engine_table: rollback_mlua::Table = lua.globals().get("Engine")?;

    engine_table.set(
        "requires_card",
        scope.create_function(|_, id: String| {
            let mut package = package.borrow_mut();
            let package_info = package.package_info_mut();
            package_info.requirements.push((PackageCategory::Card, id));

            Ok(())
        })?,
    )?;

    let requires_library = scope.create_function(|_, id: String| {
        let mut package = package.borrow_mut();
        let package_info = package.package_info_mut();
        package_info
            .requirements
            .push((PackageCategory::Library, id));

        Ok(())
    })?;

    engine_table.set("requires_character", requires_library.clone())?;
    engine_table.set("requires_library", requires_library)?;

    let define_library = scope.create_function(|lua, (id, path): (String, String)| {
        let path = absolute_path(lua, path)?;

        let mut package = package.borrow_mut();
        let package_info = package.package_info_mut();

        // add the library as a child package
        package_info.child_id_path_pairs.push((id.clone(), path));

        // add the library as a requirement
        package_info
            .requirements
            .push((PackageCategory::Library, id));

        Ok(())
    })?;

    engine_table.set("define_character", define_library.clone())?;
    engine_table.set("define_library", define_library)?;

    Ok(())
}

pub fn query_dependencies(lua: &rollback_mlua::Lua) {
    let globals = lua.globals();

    let requirement_function =
        match globals.get::<_, rollback_mlua::Function>("package_requires_scripts") {
            Ok(requirement_function) => requirement_function,
            Err(_) => return,
        };

    if let Err(e) = requirement_function.call::<_, ()>(()) {
        log::error!("{e}");
    }
}
