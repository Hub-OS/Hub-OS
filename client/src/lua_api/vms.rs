use crate::battle::{BattleScriptContext, BattleSimulation, RollbackVM};
use crate::packages::PackageInfo;
use crate::resources::{AssetManager, Globals, ResourcePaths, INPUT_BUFFER_LIMIT};
use framework::prelude::GameIO;
use std::cell::RefCell;

// 1/2 MiB
const VM_MEMORY: usize = 1024 * 1024 / 2;

pub fn create_analytical_vm(
    assets: &impl AssetManager,
    package_info: &PackageInfo,
) -> rollback_mlua::Lua {
    let lua = rollback_mlua::Lua::new();
    lua.load_from_std_lib(rollback_mlua::StdLib::MATH | rollback_mlua::StdLib::TABLE)
        .unwrap();

    {
        let globals = lua.globals();

        let stub = lua.create_function(|_, ()| Ok(())).unwrap();

        // engine stubs
        let engine_table = lua.create_table().unwrap();
        engine_table.set("load_texture", stub.clone()).unwrap();
        engine_table.set("load_audio", stub).unwrap();
        globals.set("Engine", engine_table).unwrap();

        let globals_metatable = lua.create_table().unwrap();
        globals_metatable.set("__index", lua.globals()).unwrap();
        let globals_metatable_key = lua.create_registry_value(globals_metatable).unwrap();

        // version of include that doesn't require a context
        // real include is in include_api.rs
        globals
            .set(
                "include",
                lua.create_function(move |lua, path: String| {
                    let env = lua.environment()?;
                    let folder_path: String = env.get("_folder_path")?;

                    let path = folder_path + &path;
                    let source = std::fs::read_to_string(&path).unwrap_or_default();

                    let metatable: rollback_mlua::Table =
                        lua.registry_value(&globals_metatable_key)?;

                    let env = lua.create_table()?;
                    env.set_metatable(Some(metatable));
                    env.set("_folder_path", ResourcePaths::parent(&path))?;

                    lua.load(&source)
                        .set_name(ResourcePaths::shorten(&path))?
                        .set_environment(env)?
                        .call::<_, rollback_mlua::Value>(())
                })
                .unwrap(),
            )
            .unwrap();

        // real api that does not depend on anything
        super::global_api::inject_global_api(&lua).unwrap();
    }

    if let Err(e) = load_root_script(&lua, assets, package_info) {
        log::error!(
            "{:?} {}",
            ResourcePaths::shorten(&package_info.script_path),
            e
        );
    }

    lua
}

pub fn create_battle_vm(
    game_io: &GameIO<Globals>,
    simulation: &mut BattleSimulation,
    vms: &mut Vec<RollbackVM>,
    package_info: &PackageInfo,
) -> usize {
    let globals = game_io.globals();

    let lua = rollback_mlua::Lua::new_rollback(VM_MEMORY, INPUT_BUFFER_LIMIT);
    lua.load_from_std_lib(rollback_mlua::StdLib::MATH | rollback_mlua::StdLib::TABLE)
        .unwrap();

    let vm = RollbackVM {
        lua,
        package_id: package_info.id.clone(),
        namespace: package_info.namespace,
        path: package_info.script_path.clone(),
    };

    let vm_index = vms.len();
    vms.push(vm);

    let lua = &vms.last().unwrap().lua;
    lua.set_named_registry_value("vm_index", vm_index).unwrap();
    lua.set_named_registry_value("namespace", package_info.namespace)
        .unwrap();

    globals.battle_api.inject_static(lua).unwrap();

    let api_ctx = RefCell::new(BattleScriptContext {
        vm_index,
        vms,
        game_io,
        simulation,
    });

    globals.battle_api.inject_dynamic(lua, &api_ctx, |lua| {
        load_root_script(lua, &globals.assets, package_info)
    });

    vm_index
}

fn load_root_script(
    lua: &rollback_mlua::Lua,
    assets: &impl AssetManager,
    package_info: &PackageInfo,
) -> rollback_mlua::Result<()> {
    let script_source = assets.text(&package_info.script_path);
    lua.globals().set(
        "_folder_path",
        ResourcePaths::clean_folder(&package_info.base_path),
    )?;

    lua.globals()
        .set("_game_folder_path", ResourcePaths::game_folder())?;

    let chunk = lua
        .load(&script_source)
        .set_name(ResourcePaths::shorten(&package_info.script_path))?;

    chunk.exec()?;

    Ok(())
}
