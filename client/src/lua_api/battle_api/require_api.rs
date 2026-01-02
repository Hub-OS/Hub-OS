use super::{BattleLuaApi, GLOBAL_TABLE, LOADED_REGISTRY_KEY, MODULES_REGISTRY_KEY};
use crate::resources::{AssetManager, Globals, ResourcePaths};

pub fn inject_require_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_static_injector(|lua| {
        lua.set_named_registry_value(LOADED_REGISTRY_KEY, lua.create_table()?)?;
        lua.set_named_registry_value(MODULES_REGISTRY_KEY, lua.create_table()?)?;

        Ok(())
    });

    lua_api.add_dynamic_function(GLOBAL_TABLE, "require", |api_ctx, lua, params| {
        let path: String = lua.unpack_multi(params)?;

        let api_ctx = api_ctx.borrow();
        let globals = Globals::from_resources(api_ctx.game_io);

        let path = {
            // try to resolve from library packages
            let vms = api_ctx.resources.vm_manager.vms();
            let vm = &vms[api_ctx.vm_index];
            let ns = vm.preferred_namespace();

            let package_id = path.into();
            let mut library_package = None;

            if let Some(package) = globals
                .library_packages
                .package_or_fallback(ns, &package_id)
            {
                if vm.permitted_dependencies.contains(&package_id) {
                    library_package = Some(package);
                } else {
                    log::warn!(
                        "Found package {package_id} required by {} outside of dependency tree",
                        vm.package_id
                    )
                }
            }

            if let Some(package) = library_package {
                package.package_info.script_path.clone()
            } else {
                // resolve from file
                let env = lua.environment()?;
                let folder_path: String = env.get("_folder_path")?;

                let mut path = folder_path + package_id.as_str();

                // append .lua if the path is missing the extension
                if !path.ends_with(".lua") {
                    path += ".lua";
                }

                ResourcePaths::clean(&path)
            }
        };

        // see if this module is already loaded
        let loaded_table: rollback_mlua::Table = lua.named_registry_value(LOADED_REGISTRY_KEY)?;
        let modules_table: rollback_mlua::Table = lua.named_registry_value(MODULES_REGISTRY_KEY)?;

        if let Ok(true) = loaded_table.raw_get::<_, bool>(path.as_str()) {
            // return existing module
            let module = modules_table.raw_get::<_, rollback_mlua::Table>(path)?;
            let mut multi = lua.pack_multi(())?;

            for i in (1..=module.raw_len()).rev() {
                multi.push_front(module.raw_get(i)?);
            }

            return lua.pack_multi(multi);
        };

        // mark as loaded
        loaded_table.raw_set(path.as_str(), true)?;

        let source = globals.assets.text(&path);

        // drop api_ctx to allow the module to borrow it
        std::mem::drop(api_ctx);

        let metatable = lua.globals().get_metatable();

        let env = lua.create_table()?;
        env.set_metatable(metatable);
        env.set("_folder_path", ResourcePaths::parent(&path))?;

        let result: rollback_mlua::Result<rollback_mlua::MultiValue> = lua
            .load(&source)
            .set_name(ResourcePaths::shorten(&path))
            .set_environment(env)
            .call(());

        if let Ok(multi) = &result {
            // store module
            let table = lua.create_sequence_from(multi.iter().cloned())?;
            modules_table.raw_set(path.as_str(), table)?;
        }

        result
    });
}
