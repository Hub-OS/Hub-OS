use super::{BattleCallback, BattleSimulation, Player, PlayerOverridables, SharedBattleResources};
use crate::bindable::EntityId;
use crate::packages::AugmentPackage;
use crate::structures::GenerationalIndex;
use framework::prelude::GameIO;
use packets::structures::PackageId;
use std::borrow::Cow;

#[derive(Clone)]
pub struct Augment {
    pub package_id: PackageId,
    pub level: u8,
    pub attack_boost: i8,
    pub rapid_boost: i8,
    pub charge_boost: i8,
    pub hand_size_boost: i8,
    pub priority: bool,
    pub boost_order: usize,
    pub tags: Vec<Cow<'static, str>>,
    pub overridables: PlayerOverridables,
    pub delete_callback: Option<BattleCallback>,
}

impl From<(&AugmentPackage, usize)> for Augment {
    fn from((package, level): (&AugmentPackage, usize)) -> Self {
        Self {
            package_id: package.package_info.id.clone(),
            level: level as u8,
            attack_boost: package.attack_boost,
            rapid_boost: package.rapid_boost,
            charge_boost: package.charge_boost,
            hand_size_boost: package.hand_size_boost,
            priority: package.priority,
            boost_order: 0,
            tags: package.tags.clone(),
            overridables: PlayerOverridables::default(),
            delete_callback: None,
        }
    }
}

impl Augment {
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
