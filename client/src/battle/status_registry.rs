use super::BattleCallback;
use crate::bindable::{EntityId, HitFlag, HitFlags};
use crate::lua_api::{create_status_table, BattleVmManager, HIT_FLAG_TABLE};
use crate::packages::{Package, PackageInfo, PackageNamespace};
use crate::resources::Globals;
use framework::prelude::GameIO;
use packets::structures::{PackageCategory, PackageId};

const STATUS_LIMIT: usize = 32;

struct RegisteredStatus {
    package_id: PackageId,
    namespace: PackageNamespace,
    name: String,
    flag: HitFlags,
    constructor: BattleCallback<EntityId>,
}

pub struct StatusBlocker {
    pub blocking_flag: HitFlags, // The flag that prevents the other from going through.
    pub blocked_flag: HitFlags,  // The flag that is being prevented.
}

pub struct StatusRegistry {
    next_shift: usize,
    list: Vec<RegisteredStatus>,
    blockers: Vec<StatusBlocker>,
    immobilizing_flags: Vec<HitFlags>,
    inactionable_flags: Vec<HitFlags>,
}

impl StatusRegistry {
    pub fn new() -> Self {
        Self {
            next_shift: HitFlag::BUILT_IN.len() + 1,
            list: Vec::new(),
            blockers: vec![
                StatusBlocker {
                    blocking_flag: HitFlag::FREEZE,
                    blocked_flag: HitFlag::PARALYZE,
                },
                StatusBlocker {
                    blocking_flag: HitFlag::PARALYZE,
                    blocked_flag: HitFlag::FREEZE,
                },
                StatusBlocker {
                    blocking_flag: HitFlag::CONFUSE,
                    blocked_flag: HitFlag::FREEZE,
                },
            ],
            immobilizing_flags: vec![HitFlag::PARALYZE, HitFlag::FREEZE],
            inactionable_flags: vec![HitFlag::PARALYZE, HitFlag::FREEZE, HitFlag::ROOT],
        }
    }

    pub fn init(
        &mut self,
        game_io: &GameIO,
        vm_manager: &BattleVmManager,
        dependencies: &[(&PackageInfo, PackageNamespace)],
    ) {
        let globals = game_io.resource::<Globals>().unwrap();
        let packages = &globals.status_packages;

        for (info, namespace) in dependencies {
            if info.package_category != PackageCategory::Status {
                continue;
            }

            let Some(package) = packages.package_or_override(*namespace, &info.id) else {
                continue;
            };

            if self.list.iter().any(|item| item.name == package.flag_name) {
                // ignore conflict
                continue;
            }

            if self.next_shift >= STATUS_LIMIT {
                log::error!(
                    "Failed to register {HIT_FLAG_TABLE}.{}: Too many statuses registered",
                    package.flag_name
                );
                continue;
            }

            let Some(vm_index) = vm_manager.find_vm_from_info(package.package_info()) else {
                continue;
            };

            // register new status
            let flag = 1 << self.next_shift;

            let constructor =
                BattleCallback::new(move |game_io, resources, simulation, entity_id| {
                    let result = simulation.call_global(
                        game_io,
                        resources,
                        vm_index,
                        "status_init",
                        |lua| create_status_table(lua, entity_id, flag),
                    );

                    if let Err(err) = result {
                        log::error!("{err}");
                    }
                });

            self.list.push(RegisteredStatus {
                package_id: info.id.clone(),
                namespace: *namespace,
                name: package.flag_name.clone(),
                flag,
                constructor,
            });

            if package.blocks_actions {
                self.inactionable_flags.push(flag);
            }

            if package.blocks_mobility {
                self.immobilizing_flags.push(flag);
            }

            self.next_shift += 1;
        }

        // create inter status rules after flags are resolved
        for item in &self.list {
            let package = packages
                .package_or_override(item.namespace, &item.package_id)
                .unwrap();

            for name in &package.blocked_by {
                let flag = HitFlag::from_str(self, name);

                self.blockers.push(StatusBlocker {
                    blocking_flag: flag,
                    blocked_flag: item.flag,
                });
            }

            for name in &package.blocks_flags {
                let flag = HitFlag::from_str(self, name);

                self.blockers.push(StatusBlocker {
                    blocking_flag: item.flag,
                    blocked_flag: flag,
                });
            }
        }
    }

    pub fn resolve_flag(&self, s: &str) -> Option<HitFlags> {
        self.list
            .iter()
            .find(|item| item.name == s)
            .map(|item| item.flag)
    }

    pub fn flags(&self) -> impl Iterator<Item = HitFlags> + '_ {
        self.list.iter().map(|item| item.flag)
    }

    pub fn status_constructor(&self, flag: HitFlags) -> Option<BattleCallback<EntityId>> {
        let item = self.list.iter().find(|item| item.flag == flag)?;
        Some(item.constructor.clone())
    }

    pub fn blockers(&self) -> &[StatusBlocker] {
        &self.blockers
    }

    pub fn immobilizing_flags(&self) -> &[HitFlags] {
        &self.immobilizing_flags
    }

    pub fn inactionable_flags(&self) -> &[HitFlags] {
        &self.inactionable_flags
    }
}
