use super::{BattleCallback, PlayerOverridables};
use crate::packages::AugmentPackage;
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
            tags: package.tags.clone(),
            overridables: PlayerOverridables::default(),
            delete_callback: None,
        }
    }
}
