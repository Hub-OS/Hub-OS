use super::BattleCallback;
use crate::bindable::GenerationalIndex;
use crate::packages::BlockPackage;
use crate::render::FrameTime;

#[derive(Clone)]
pub struct AbilityModifier {
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

impl Default for AbilityModifier {
    fn default() -> Self {
        Self {
            level: 1,
            attack_boost: 0,
            rapid_boost: 0,
            charge_boost: 0,
            calculate_charge_time_callback: None,
            normal_attack_callback: None,
            charged_attack_callback: None,
            special_attack_callback: None,
            delete_callback: BattleCallback::default(),
        }
    }
}

impl From<&BlockPackage> for AbilityModifier {
    fn from(package: &BlockPackage) -> Self {
        Self {
            attack_boost: package.attack_boost,
            rapid_boost: package.rapid_boost,
            charge_boost: package.charge_boost,
            ..Default::default()
        }
    }
}
