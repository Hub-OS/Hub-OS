use super::{GAME_FOLDER_KEY, NAMESPACE_REGISTRY_KEY, VM_INDEX_REGISTRY_KEY};
use crate::battle::{BattleScriptContext, BattleSimulation, RollbackVM};
use crate::packages::PackageInfo;
use crate::resources::{AssetManager, Globals, ResourcePaths, INPUT_BUFFER_LIMIT};
use framework::prelude::GameIO;
use std::cell::RefCell;

// 1 MiB
const VM_MEMORY: usize = 1024 * 1024;

pub fn create_battle_vm(
    game_io: &GameIO,
    simulation: &mut BattleSimulation,
    vms: &mut Vec<RollbackVM>,
    package_info: &PackageInfo,
) -> usize {
    let globals = game_io.resource::<Globals>().unwrap();

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
    lua.set_named_registry_value(VM_INDEX_REGISTRY_KEY, vm_index)
        .unwrap();
    lua.set_named_registry_value(NAMESPACE_REGISTRY_KEY, package_info.namespace)
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

    lua.set_named_registry_value(GAME_FOLDER_KEY, ResourcePaths::game_folder())?;

    let chunk = lua
        .load(&script_source)
        .set_name(ResourcePaths::shorten(&package_info.script_path))?;

    chunk.exec()?;

    Ok(())
}
