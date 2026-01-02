use super::{
    GAME_FOLDER_REGISTRY_KEY, PACKAGE_ID_REGISTRY_KEY, VM_INDEX_REGISTRY_KEY,
    inject_internal_scripts,
};
use crate::battle::{
    BattleCallback, BattleScriptContext, BattleSimulation, RollbackVM, SharedBattleResources,
};
use crate::bindable::EntityId;
use crate::packages::{PackageInfo, PackageNamespace};
use crate::resources::{
    AssetManager, BATTLE_VM_MEMORY, Globals, INPUT_BUFFER_LIMIT, ResourcePaths,
};
use framework::prelude::GameIO;
use packets::structures::PackageId;
use std::cell::RefCell;
use std::collections::HashMap;

#[derive(Default)]
pub struct InternalScripts {
    pub default_player_delete: BattleCallback<EntityId>,
    pub default_character_delete: BattleCallback<(EntityId, Option<usize>)>,
    pub queue_default_player_movement: BattleCallback<(EntityId, (i32, i32))>,
}

pub struct BattleVmManager {
    pub encounter_vm: Option<usize>,
    pub vms: Vec<RollbackVM>,
    pub vms_by_id: HashMap<PackageId, Vec<usize>>,
    pub scripts: InternalScripts,
}

impl BattleVmManager {
    pub fn new() -> Self {
        Self {
            encounter_vm: None,
            vms: Vec::new(),
            vms_by_id: Default::default(),
            scripts: Default::default(),
        }
    }

    pub fn init_dependency_vms<'a>(
        game_io: &GameIO,
        resources: &mut SharedBattleResources,
        simulation: &mut BattleSimulation,
        dependencies: impl Iterator<Item = &'a (&'a PackageInfo, PackageNamespace)>,
    ) {
        for (package_info, namespace) in dependencies {
            if package_info.category.requires_vm() {
                Self::ensure_vm(game_io, resources, simulation, package_info, *namespace);
            }
        }
    }

    pub fn init_internal_vm(
        game_io: &GameIO,
        resources: &mut SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let internal_vm_package_info = PackageInfo {
            namespace: PackageNamespace::BuiltIn,
            id: PackageId::from("dev.hubos.InternalScripts"),
            ..Default::default()
        };

        Self::ensure_vm(
            game_io,
            resources,
            simulation,
            &internal_vm_package_info,
            internal_vm_package_info.namespace,
        );

        if let Err(err) = inject_internal_scripts(&mut resources.vm_manager) {
            log::error!("{err}");
        }
    }

    fn ensure_vm(
        game_io: &GameIO,
        resources: &mut SharedBattleResources,
        simulation: &mut BattleSimulation,
        package_info: &PackageInfo,
        namespace: PackageNamespace,
    ) {
        let existing_vm = resources.vm_manager.find_vm_from_info(package_info);

        if let Some(vm_index) = existing_vm {
            // recycle a vm
            let vm = &mut resources.vm_manager.vms[vm_index];

            if !vm.namespaces.contains(&namespace) {
                vm.namespaces.push(namespace);
            }

            return;
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
        let globals = Globals::from_resources(game_io);

        let lua = rollback_mlua::Lua::new_rollback(BATTLE_VM_MEMORY, INPUT_BUFFER_LIMIT);
        lua.load_from_std_lib(
            rollback_mlua::StdLib::MATH
                | rollback_mlua::StdLib::TABLE
                | rollback_mlua::StdLib::STRING,
        )
        .unwrap();

        let vm = RollbackVM {
            lua,
            package_id: package_info.id.clone(),
            match_namespace: package_info.namespace,
            namespaces: vec![namespace],
            path: package_info.script_path.clone(),
            permitted_dependencies: globals
                .package_dependency_iter([(
                    package_info.category,
                    package_info.namespace,
                    package_info.id.clone(),
                )])
                .iter()
                .map(|(info, _)| info.id.clone())
                .collect(),
        };

        // track vm
        let vm_manager = &mut resources.vm_manager;
        let vm_index = vm_manager.vms.len();

        let by_id_entry = vm_manager
            .vms_by_id
            .entry(vm.package_id.clone())
            .or_default();
        by_id_entry.push(vm_index);

        vm_manager.vms.push(vm);

        // load root script
        let vms = &resources.vm_manager.vms;
        let lua = &vms.last().unwrap().lua;

        if lua
            .set_named_registry_value(VM_INDEX_REGISTRY_KEY, vm_index)
            .is_err()
        {
            log::error!(
                "Failed to set VM_INDEX_REGISTRY_KEY for {}",
                package_info.id
            );
            return;
        }

        if lua
            .set_named_registry_value(PACKAGE_ID_REGISTRY_KEY, package_info.id.as_str())
            .is_err()
        {
            log::error!(
                "Failed to set PACKAGE_ID_REGISTRY_KEY for {}",
                package_info.id
            );
            return;
        }

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
        self.vms_by_id
            .get(&package_info.id)
            .into_iter()
            .flat_map(|vm_indices| vm_indices.iter())
            .find(|i| self.vms[**i].match_namespace == package_info.namespace)
            .cloned()
    }

    pub fn find_vm(
        &self,
        package_id: &PackageId,
        namespace: PackageNamespace,
    ) -> rollback_mlua::Result<usize> {
        let vm_index = self
            .vms_by_id
            .get(package_id)
            .and_then(|vm_indices| {
                namespace.find_with_fallback(|namespace| {
                    vm_indices
                        .iter()
                        .find(|&&vm_index| self.vms[vm_index].namespaces.contains(&namespace))
                })
            })
            .ok_or_else(|| {
                rollback_mlua::Error::RuntimeError(format!(
                    "no package with id {package_id:?} and namespace {namespace:?} found"
                ))
            })?;

        Ok(*vm_index)
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
        println!(
            "| {:-<NAMESPACE_WIDTH$} | {:-<package_id_width$} | {:-<MEMORY_WIDTH$} | {:-<MEMORY_WIDTH$} |",
            "", "", "", ""
        );

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

    lua.set_named_registry_value(GAME_FOLDER_REGISTRY_KEY, ResourcePaths::game_folder())?;

    let chunk = lua
        .load(&script_source)
        .set_name(ResourcePaths::shorten(&package_info.script_path));

    chunk.exec()?;

    Ok(())
}
