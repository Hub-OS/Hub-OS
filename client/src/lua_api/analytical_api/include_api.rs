use crate::resources::{AssetManager, LocalAssetManager, ResourcePaths};

// the real include is in include_api.rs, this is just for the analytical vm
pub fn inject_package_management_api<'lua: 'scope, 'scope: 'closure, 'closure>(
    lua: &'lua rollback_mlua::Lua,
    scope: &'closure rollback_mlua::Scope<'lua, 'scope>,
    assets: &'scope LocalAssetManager,
) -> rollback_mlua::Result<()> {
    let globals = lua.globals();

    globals.set(
        "include",
        scope.create_function(move |lua, path: String| {
            let env = lua.environment()?;
            let folder_path: String = env.get("_folder_path")?;

            let path = folder_path + &path;
            let source = assets.text(&path);

            let metatable: rollback_mlua::Table = lua.named_registry_value("globals")?;

            let env = lua.create_table()?;
            env.set_metatable(Some(metatable));
            env.set("_folder_path", ResourcePaths::parent(&path))?;

            lua.load(&source)
                .set_name(ResourcePaths::shorten(&path))?
                .set_environment(env)?
                .call::<_, rollback_mlua::Value>(())
        })?,
    )?;

    Ok(())
}
