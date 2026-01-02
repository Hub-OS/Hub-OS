use std::cell::RefCell;

use crate::battle::{BattleScriptContext, BattleSimulation, SharedBattleResources};
use crate::bindable::EntityId;
use crate::lua_api::create_entity_table;
use crate::packages::PackageNamespace;
use crate::resources::Globals;
use framework::common::GameIO;
use packets::structures::PackageId;

pub struct CardDamageResolver {
    vm_index: Option<usize>,
    default_damage: i32,
}

impl CardDamageResolver {
    pub fn new(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        namespace: PackageNamespace,
        package_id: &PackageId,
    ) -> Self {
        let globals = Globals::from_resources(game_io);
        let Some(card_package) = globals
            .card_packages
            .package_or_fallback(namespace, package_id)
        else {
            return Self {
                vm_index: None,
                default_damage: 0,
            };
        };

        let default_damage = card_package.card_properties.damage;

        if !card_package.card_properties.dynamic_damage {
            return Self {
                vm_index: None,
                default_damage,
            };
        }

        Self {
            vm_index: resources.vm_manager.find_vm(package_id, namespace).ok(),
            default_damage,
        }
    }

    pub fn needs_resolving(&self) -> bool {
        self.vm_index.is_some()
    }

    pub fn resolve(
        self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) -> i32 {
        let globals = Globals::from_resources(game_io);
        let Some(vm_index) = self.vm_index else {
            return self.default_damage;
        };

        let vms = resources.vm_manager.vms();
        let lua = &vms[vm_index].lua;
        let Ok(card_init) = lua
            .globals()
            .get::<_, rollback_mlua::Function>("card_dynamic_damage")
        else {
            return self.default_damage;
        };

        let api_ctx = RefCell::new(BattleScriptContext {
            vm_index,
            resources,
            game_io,
            simulation,
        });

        let lua_api = &globals.battle_api;
        let mut damage = self.default_damage;

        lua_api.inject_dynamic(lua, &api_ctx, |lua| {
            let entity_table = create_entity_table(lua, entity_id)?;

            match card_init.call(entity_table) {
                Ok(Some(result)) => damage = result,
                Ok(None) => {}
                Err(err) => {
                    log::error!("{err}");
                    return Ok(());
                }
            }

            Ok(())
        });

        damage
    }
}
