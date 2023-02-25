use packets::structures::PackageId;

use super::BattleCallback;
use crate::bindable::GenerationalIndex;
use crate::packages::AugmentPackage;
use crate::render::FrameTime;

#[derive(Clone)]
pub struct Augment {
    pub package_id: PackageId,
    pub level: u8,
    pub attack_boost: i8,
    pub rapid_boost: i8,
    pub charge_boost: i8,
    pub calculate_charge_time_callback: Option<BattleCallback<u8, FrameTime>>,
    pub normal_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub charged_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub special_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub delete_callback: BattleCallback,
}

impl From<(&AugmentPackage, usize)> for Augment {
    fn from((package, level): (&AugmentPackage, usize)) -> Self {
        Self {
            package_id: package.package_info.id.clone(),
            level: level as u8,
            attack_boost: package.attack_boost,
            rapid_boost: package.rapid_boost,
            charge_boost: package.charge_boost,
            calculate_charge_time_callback: None,
            normal_attack_callback: None,
            charged_attack_callback: None,
            special_attack_callback: None,
            delete_callback: BattleCallback::default(),
        }
    }
}
