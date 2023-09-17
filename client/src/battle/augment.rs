use super::BattleCallback;
use crate::bindable::{CardProperties, GenerationalIndex};
use crate::packages::AugmentPackage;
use crate::render::FrameTime;
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
    pub calculate_charge_time_callback: Option<BattleCallback<u8, FrameTime>>,
    pub normal_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub charged_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub special_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub can_charge_card_callback: Option<BattleCallback<CardProperties, bool>>,
    pub charged_card_callback: Option<BattleCallback<CardProperties, Option<GenerationalIndex>>>,
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
            calculate_charge_time_callback: None,
            normal_attack_callback: None,
            charged_attack_callback: None,
            special_attack_callback: None,
            can_charge_card_callback: None,
            charged_card_callback: None,
            delete_callback: None,
        }
    }
}
