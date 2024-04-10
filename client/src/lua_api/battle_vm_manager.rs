use super::{GAME_FOLDER_KEY, VM_INDEX_REGISTRY_KEY};
use crate::battle::{BattleScriptContext, BattleSimulation, RollbackVM, SharedBattleResources};
use crate::packages::{PackageInfo, PackageNamespace};
use crate::resources::{
    AssetManager, Globals, ResourcePaths, BATTLE_VM_MEMORY, INPUT_BUFFER_LIMIT,
};
use framework::prelude::GameIO;
use packets::structures::PackageId;
use std::cell::RefCell;

pub struct BattleVmManager {
    vms: Vec<RollbackVM>,
}

impl BattleVmManager {
    pub fn new() -> Self {
        Self { vms: Vec::new() }
    }

    pub fn init(
        game_io: &GameIO,
        resources: &mut SharedBattleResources,
        simulation: &mut BattleSimulation,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) {
        for (package_info, namespace) in dependencies {
            if package_info.package_category.requires_vm() {
                Self::ensure_vm(game_io, resources, simulation, package_info, *namespace);
            }
        }
    }

    fn ensure_vm(
        game_io: &GameIO,
        resources: &mut SharedBattleResources,
        simulation: &mut BattleSimulation,
        package_info: &PackageInfo,
        namespace: PackageNamespace,
    ) {
        // expecting local namespace to be converted to a Netplay namespace
        // before being passed to this function
        assert_ne!(namespace, PackageNamespace::Local);

        let existing_vm = resources.vm_manager.find_vm_from_info(package_info);

        if let Some(vm_index) = existing_vm {
            // recycle a vm
            let vm = &mut resources.vm_manager.vms[vm_index];

            if !vm.namespaces.contains(&namespace) {
                vm.namespaces.push(namespace);
            }
        }

        // new vm
        Self::create_vm(game_io, resources, simulation, package_info, namespace)
    }

    fn create_vm(
        game_io: &GameIO,
        resources: &mut SharedBattleResources,
        simulation: &mut BattleSimulation,
        package_info: &PackageInfo,
        namespace: PackageNamespace,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();

        let lua = rollback_mlua::Lua::new_rollback(BATTLE_VM_MEMORY, INPUT_BUFFER_LIMIT);
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

        let vms = &mut resources.vm_manager.vms;
        let vm_index = vms.len();
        vms.push(vm);

        let vms = &resources.vm_manager.vms;
        let lua = &vms.last().unwrap().lua;
        lua.set_named_registry_value(VM_INDEX_REGISTRY_KEY, vm_index)
            .unwrap();

        globals.battle_api.inject_static(lua).unwrap();

        let api_ctx = RefCell::new(BattleScriptContext {
            vm_index,
            resources,
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

    pub fn find_vm_from_info(&self, package_info: &PackageInfo) -> Option<usize> {
        self.vms
            .iter()
            .position(|vm| vm.path == package_info.script_path)
    }

    pub fn find_vm(
        &self,
        package_id: &PackageId,
        namespace: PackageNamespace,
    ) -> rollback_mlua::Result<usize> {
        let vm_index = namespace
            .find_with_overrides(|namespace| {
                self.vms.iter().position(|vm| {
                    vm.package_id == *package_id && vm.namespaces.contains(&namespace)
                })
            })
            .ok_or_else(|| {
                rollback_mlua::Error::RuntimeError(format!(
                    "no package with id {:?} found",
                    package_id
                ))
            })?;

        Ok(vm_index)
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
            // todo: maybe we should list every namespace in a compressed format
            // ex: [RecordingServer, Server, Local, Netplay(0)] -> R S L 0
            let ns = vm.preferred_namespace();
            let ns_string = match ns {
                PackageNamespace::RecordingServer => String::from("RecServer"),
                _ => format!("{ns:?}"),
            };

            println!(
                "| {:NAMESPACE_WIDTH$} | {:package_id_width$} | {:MEMORY_WIDTH$} | {:MEMORY_WIDTH$} |",
                ns_string,
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
        .set_name(ResourcePaths::shorten(&package_info.script_path));

    chunk.exec()?;

    Ok(())
}
