use super::{GAME_FOLDER_KEY, VM_INDEX_REGISTRY_KEY};
use crate::battle::{BattleScriptContext, BattleSimulation, RollbackVM};
use crate::packages::{PackageInfo, PackageNamespace};
use crate::resources::{AssetManager, Globals, ResourcePaths, INPUT_BUFFER_LIMIT};
use framework::prelude::GameIO;
use std::cell::RefCell;

// 1 MiB
const VM_MEMORY: usize = 1024 * 1024;

pub struct BattleVmManager {
    vms: Vec<RollbackVM>,
}

impl BattleVmManager {
    pub fn new(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) -> Self {
        let mut manager = Self { vms: Vec::new() };

        manager.init(game_io, simulation, dependencies);

        manager
    }

    fn init(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) {
        for (package_info, namespace) in dependencies {
            if package_info.package_category.requires_vm() {
                self.ensure_vm(game_io, simulation, package_info, *namespace);
            }
        }
    }

    fn ensure_vm(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        package_info: &PackageInfo,
        namespace: PackageNamespace,
    ) {
        // expecting local namespace to be converted to a Netplay namespace
        // before being passed to this function
        assert_ne!(namespace, PackageNamespace::Local);

        let existing_vm = self.find_vm(package_info);

        if let Some(vm_index) = existing_vm {
            // recycle a vm
            let vm = &mut self.vms[vm_index];

            if !vm.namespaces.contains(&namespace) {
                vm.namespaces.push(namespace);
            }
        }

        // new vm
        self.create_vm(game_io, simulation, package_info, namespace)
    }

    fn create_vm(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        package_info: &PackageInfo,
        namespace: PackageNamespace,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();

        let lua = rollback_mlua::Lua::new_rollback(VM_MEMORY, INPUT_BUFFER_LIMIT);
        lua.load_from_std_lib(rollback_mlua::StdLib::MATH | rollback_mlua::StdLib::TABLE)
            .unwrap();

        let vm = RollbackVM {
            lua,
            package_id: package_info.id.clone(),
            namespaces: if namespace == package_info.namespace {
                vec![namespace]
            } else {
                vec![namespace, package_info.namespace]
            },
            path: package_info.script_path.clone(),
        };

        let vm_index = self.vms.len();
        self.vms.push(vm);

        let lua = &self.vms.last().unwrap().lua;
        lua.set_named_registry_value(VM_INDEX_REGISTRY_KEY, vm_index)
            .unwrap();

        globals.battle_api.inject_static(lua).unwrap();

        let api_ctx = RefCell::new(BattleScriptContext {
            vm_index,
            vms: &self.vms,
            game_io,
            simulation,
        });

        globals.battle_api.inject_dynamic(lua, &api_ctx, |lua| {
            load_root_script(lua, &globals.assets, package_info)
        });
    }

    pub fn vms(&self) -> &[RollbackVM] {
        &self.vms
    }

    pub fn find_vm(&self, package_info: &PackageInfo) -> Option<usize> {
        self.vms
            .iter()
            .position(|vm| vm.path == package_info.script_path)
    }

    pub fn snap(&mut self) {
        for vm in &mut self.vms {
            vm.lua.snap();
        }
    }

    pub fn rollback(&mut self, n: usize) {
        for vm in &mut self.vms {
            vm.lua.rollback(n);
        }
    }

    pub fn print_memory_usage(&self) {
        const NAMESPACE_WIDTH: usize = 10;
        const MEMORY_WIDTH: usize = 11;

        let mut vms_sorted: Vec<_> = self.vms.iter().collect();
        vms_sorted.sort_by_key(|vm| vm.lua.unused_memory());

        let package_id_width = vms_sorted
            .iter()
            .map(|vm| vm.package_id.as_str().len())
            .max()
            .unwrap_or_default()
            .max(10);

        // margin + header
        println!(
            "\n| Namespace  | Package ID{:1$} | Used Memory | Free Memory |",
            " ",
            package_id_width - 10
        );

        // border
        println!("| {:-<NAMESPACE_WIDTH$} | {:-<package_id_width$} | {:-<MEMORY_WIDTH$} | {:-<MEMORY_WIDTH$} |", "", "", "", "");

        // list
        for vm in vms_sorted {
            println!(
                "| {:NAMESPACE_WIDTH$} | {:package_id_width$} | {:MEMORY_WIDTH$} | {:MEMORY_WIDTH$} |",
                // todo: maybe we should list every namespace in a compressed format
                // ex: [Server, Local, Netplay(0)] -> S L 0
                format!("{:?}", vm.preferred_namespace()),
                vm.package_id,
                vm.lua.used_memory(),
                vm.lua.unused_memory().unwrap()
            );
        }

        // margin
        println!();
    }
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
