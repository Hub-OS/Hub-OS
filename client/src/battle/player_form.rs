use super::BattleCallback;
use crate::bindable::GenerationalIndex;
use crate::render::FrameTime;
use framework::prelude::Texture;
use std::sync::Arc;

#[derive(Default, Clone)]
pub struct PlayerForm {
    pub activated: bool,
    pub deactivated: bool,
    pub mug_texture: Option<Arc<Texture>>,
    pub activate_callback: Option<BattleCallback>,
    pub deactivate_callback: Option<BattleCallback>,
    pub update_callback: Option<BattleCallback>,
    pub calculate_charge_time_callback: Option<BattleCallback<u8, FrameTime>>,
    pub charged_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub special_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
}
