use super::BattleCallback;
use crate::bindable::{EntityId, HitFlag, HitFlags};
use crate::lua_api::{BattleVmManager, HIT_FLAG_TABLE, create_status_table};
use crate::packages::{Package, PackageInfo, PackageNamespace};
use crate::render::FrameTime;
use crate::resources::Globals;
use crate::structures::VecSet;
use framework::prelude::GameIO;
use packets::structures::{PackageCategory, PackageId};
use std::collections::HashMap;
use std::sync::Arc;

const STATUS_LIMIT: HitFlags = 60;

pub struct RegisteredStatus {
    pub package_id: PackageId,
    pub namespace: PackageNamespace,
    pub name: Arc<str>,
    pub flag: HitFlags,
    pub durations: Vec<FrameTime>,
    pub constructor: BattleCallback<(EntityId, bool)>,
}

impl RegisteredStatus {
    pub fn duration_for(&self, mut level: usize) -> FrameTime {
        level = level.saturating_sub(1);

        *self
            .durations
            .get(level)
            .or(self.durations.last())
            .unwrap_or(&1)
    }
}

pub struct StatusRegistry {
    next_shift: HitFlags,
    list: Vec<RegisteredStatus>,
    mutual_exclusions: HashMap<HitFlags, VecSet<HitFlags>>,
    overrides: HashMap<HitFlags, HitFlags>,
    immobilizing_flags: HitFlags,
    inactionable_flags: HitFlags,
}

impl StatusRegistry {
    pub fn new() -> Self {
        Self {
            next_shift: HitFlag::NEXT_SHIFT,
            list: Vec::new(),
            mutual_exclusions: Default::default(),
            overrides: Default::default(),
            immobilizing_flags: HitFlag::NONE,
            inactionable_flags: HitFlag::NONE,
        }
    }

    pub fn init(
        &mut self,
        game_io: &GameIO,
        vm_manager: &BattleVmManager,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) {
        let globals = Globals::from_resources(game_io);
        let packages = &globals.status_packages;

        for (info, namespace) in dependencies {
            if info.category != PackageCategory::Status {
                continue;
            }

            let Some(package) = packages.package_or_fallback(*namespace, &info.id) else {
                continue;
            };

            let existing_index = self
                .list
                .iter()
                .position(|item| item.name == package.flag_name);

            if existing_index.is_some_and(|i| {
                !matches!(
                    self.list[i].namespace,
                    PackageNamespace::BuiltIn | PackageNamespace::RecordingBuiltIn,
                )
            }) {
                // skip conflict
                continue;
            }

            let Some(vm_index) = vm_manager.find_vm_from_info(package.package_info()) else {
                continue;
            };

            // register new status
            let mut flag = HitFlag::from_str(self, &package.flag_name);

            if flag != HitFlag::NONE {
                // overwrite existing flag
                self.immobilizing_flags &= !flag;
                self.inactionable_flags &= !flag;
            } else {
                // use a new flag
                if self.next_shift >= STATUS_LIMIT {
                    log::error!(
                        "Failed to register {HIT_FLAG_TABLE}.{}: Too many statuses registered",
                        package.flag_name
                    );
                    continue;
                }

                flag = 1 << self.next_shift;
                self.next_shift += 1;
            }

            let constructor = BattleCallback::new(
                move |game_io, resources, simulation, (entity_id, reapplied)| {
                    let result = simulation.call_global(
                        game_io,
                        resources,
                        vm_index,
                        "status_init",
                        |lua| create_status_table(lua, entity_id, flag, reapplied),
                    );

                    if let Err(err) = result {
                        log::error!("{err}");
                    }
                },
            );

            let status = RegisteredStatus {
                package_id: info.id.clone(),
                namespace: *namespace,
                name: package.flag_name.clone(),
                flag,
                durations: package.durations.clone(),
                constructor,
            };

            if let Some(i) = existing_index {
                self.list[i] = status;
            } else {
                self.list.push(status);
            }

            if package.blocks_actions {
                self.inactionable_flags |= flag;
            }

            if package.blocks_mobility {
                self.immobilizing_flags |= flag;
            }
        }

        // create inter status rules after flags are resolved
        for item in &self.list {
            let package = packages
                .package_or_fallback(item.namespace, &item.package_id)
                .unwrap();

            for name in &package.mutual_exclusions {
                let flag = HitFlag::from_str(self, name);

                let set = self.mutual_exclusions.entry(item.flag).or_default();
                set.insert(flag);

                let set = self.mutual_exclusions.entry(flag).or_default();
                set.insert(item.flag);
            }

            for name in &package.blocked_by {
                let flag = HitFlag::from_str(self, name);
                let overrides = self.overrides.entry(item.flag).or_default();
                *overrides |= flag;
            }

            for name in &package.blocks_flags {
                let flag = HitFlag::from_str(self, name);
                let overrides = self.overrides.entry(flag).or_default();
                *overrides |= item.flag;
            }
        }
    }

    pub fn resolve_flag(&self, s: &str) -> Option<HitFlags> {
        self.list
            .iter()
            .find(|item| &*item.name == s)
            .map(|item| item.flag)
    }

    pub fn registered_list(&self) -> &[RegisteredStatus] {
        &self.list
    }

    pub fn duration_for(&self, flag: HitFlags, level: usize) -> FrameTime {
        self.registered_list()
            .iter()
            .find(|status| status.flag == flag)
            .map(|status| status.duration_for(level))
            .unwrap_or(1)
    }

    pub fn status_constructor(&self, flag: HitFlags) -> Option<BattleCallback<(EntityId, bool)>> {
        let item = self.list.iter().find(|item| item.flag == flag)?;
        Some(item.constructor.clone())
    }

    pub fn mutual_exclusions_for(&self, flag: HitFlags) -> &[HitFlags] {
        let Some(set) = self.mutual_exclusions.get(&flag) else {
            return &[];
        };

        set
    }

    pub fn overrides_for(&self, flag: HitFlags) -> HitFlags {
        self.overrides.get(&flag).cloned().unwrap_or_default()
    }

    pub fn immobilizing_flags(&self) -> HitFlags {
        self.immobilizing_flags
    }

    pub fn inactionable_flags(&self) -> HitFlags {
        self.inactionable_flags
    }
}
