use super::{BattleLuaApi, GLOBAL_TABLE};
use crate::resources::{AssetManager, Globals, ResourcePaths};

pub fn inject_require_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(GLOBAL_TABLE, "require", |api_ctx, lua, params| {
        let path: String = lua.unpack_multi(params)?;

        let source_path: String;

        let api_ctx = api_ctx.borrow();
        let globals = api_ctx.game_io.resource::<Globals>().unwrap();

        // try to resolve from library packages
        let vms = api_ctx.resources.vm_manager.vms();
        let ns = vms[api_ctx.vm_index].preferred_namespace();

        let package_id = path.into();

        if let Some(package) = globals
            .library_packages
            .package_or_fallback(ns, &package_id)
        {
            source_path = package.package_info.script_path.clone();
        } else {
            // resolve from file
            let env = lua.environment()?;
            let folder_path: String = env.get("_folder_path")?;
            source_path = ResourcePaths::clean(&(folder_path + package_id.as_str()));
        }

        let source = globals.assets.text(&source_path);

        let metatable = lua.globals().get_metatable();

        let env = lua.create_table()?;
        env.set_metatable(metatable);
        env.set("_folder_path", ResourcePaths::parent(&source_path))?;

        lua.load(&source)
            .set_name(ResourcePaths::shorten(&source_path))
            .set_environment(env)
            .call(())
    });
}
