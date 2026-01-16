use super::{BattleCallback, BattleSimulation, Player, PlayerOverridables, SharedBattleResources};
use crate::bindable::EntityId;
use crate::lua_api::create_augment_table;
use crate::packages::{AugmentPackage, PackageNamespace};
use crate::structures::GenerationalIndex;
use framework::prelude::GameIO;
use packets::structures::PackageId;

#[derive(Clone)]
pub struct Augment {
    pub package_id: PackageId,
    pub namespace: PackageNamespace,
    pub level: u8,
    pub attack_boost: i8,
    pub rapid_boost: i8,
    pub charge_boost: i8,
    pub hand_size_boost: i8,
    pub priority: bool,
    pub boost_order: usize,
    pub overridables: PlayerOverridables,
    pub delete_callback: Option<BattleCallback>,
}

impl From<(&AugmentPackage, usize)> for Augment {
    fn from((package, level): (&AugmentPackage, usize)) -> Self {
        Self {
            package_id: package.package_info.id.clone(),
            namespace: package.package_info.namespace,
            level: level as u8,
            attack_boost: package.attack_boost,
            rapid_boost: package.rapid_boost,
            charge_boost: package.charge_boost,
            hand_size_boost: package.hand_size_boost,
            priority: package.priority,
            boost_order: 0,
            overridables: PlayerOverridables::default(),
            delete_callback: None,
        }
    }
}

impl Augment {
    pub fn boost(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        id: EntityId,
        package: &AugmentPackage,
        namespace: PackageNamespace,
        level_boost: i32,
    ) {
        let entities = &mut simulation.entities;

        let Ok(player) = entities.query_one_mut::<&mut Player>(id.into()) else {
            log::error!("Augment::boost() called with invalid entity?");
            return;
        };

        let boost_order = player.augments.len();
        let mut augment_iter = player.augments.iter_mut();
        let existing_augment =
            augment_iter.find(|(_, augment)| augment.package_id == package.package_info.id);

        if let Some((index, augment)) = existing_augment {
            // boost existing augment
            let updated_level = (augment.level as i32 + level_boost).clamp(0, 100);
            augment.level = updated_level as u8;
            let prev_order = augment.boost_order;
            augment.boost_order = boost_order;

            // adjust boost order for other augments
            for augment in player.augments.values_mut() {
                if augment.boost_order > prev_order {
                    augment.boost_order -= 1;
                }
            }

            if player.form_boost_order > prev_order {
                player.form_boost_order -= 1;
            }

            if updated_level == 0 {
                // delete
                Augment::delete(game_io, resources, simulation, id, index);
            }

            return;
        }

        if level_boost <= 0 {
            return;
        }

        // create
        let package_info = &package.package_info;

        let vm_manager = &resources.vm_manager;

        let mut augment = Augment::from((package, level_boost as usize));
        augment.boost_order = boost_order;

        let index = player.augments.insert(augment);

        let Ok(vm_index) = vm_manager.find_vm(&package_info.id, namespace) else {
            return;
        };

        let vms = vm_manager.vms();
        let lua = &vms[vm_index].lua;
        let has_init = lua
            .globals()
            .contains_key("augment_init")
            .unwrap_or_default();

        if !has_init {
            return;
        }

        let result =
            simulation.call_global(game_io, resources, vm_index, "augment_init", move |lua| {
                create_augment_table(lua, id, index)
            });

        if let Err(e) = result {
            log::error!("{e}");
        }
    }

    pub fn delete(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        index: GenerationalIndex,
    ) {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&mut Player>(entity_id.into()) else {
            return;
        };

        let augment = player.augments.remove(index).unwrap();

        let overridables = augment.overridables;
        overridables.delete_self(&mut simulation.sprite_trees, &mut simulation.animators);

        if let Some(callback) = augment.delete_callback {
            callback.call(game_io, resources, simulation, ());
        }
    }
}
