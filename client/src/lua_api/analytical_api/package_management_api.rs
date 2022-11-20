use crate::lua_api::helpers::absolute_path;
use crate::packages::{Package, PackageCategory};
use std::cell::RefCell;

pub fn inject_package_management_api<'lua: 'scope, 'scope: 'closure, 'closure>(
    lua: &'lua rollback_mlua::Lua,
    scope: &'closure rollback_mlua::Scope<'lua, 'scope>,
    package: &'lua RefCell<impl Package + 'static>,
) -> rollback_mlua::Result<()> {
    let engine_table: rollback_mlua::Table = lua.globals().get("Engine")?;

    let generate_require_function = |package_category| {
        scope.create_function(move |_, id: String| {
            let mut package = package.borrow_mut();
            let package_info = package.package_info_mut();
            package_info.requirements.push((package_category, id));

            Ok(())
        })
    };

    engine_table.set(
        "requires_card",
        generate_require_function(PackageCategory::Card)?,
    )?;

    engine_table.set(
        "requires_character",
        generate_require_function(PackageCategory::Character)?,
    )?;

    engine_table.set(
        "requires_library",
        generate_require_function(PackageCategory::Library)?,
    )?;

    engine_table.set(
        "define_character",
        scope.create_function(|lua, (id, path): (String, String)| {
            let path = absolute_path(lua, path)?;

            let mut package = package.borrow_mut();
            let package_info = package.package_info_mut();

            // add the character as a child package
            package_info.child_id_path_pairs.push((id.clone(), path));

            // add the character as a requirement
            package_info
                .requirements
                .push((PackageCategory::Character, id));

            Ok(())
        })?,
    )?;

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
