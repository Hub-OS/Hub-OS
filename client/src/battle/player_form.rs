use super::BattleCallback;
use crate::bindable::{CardProperties, GenerationalIndex};
use crate::render::FrameTime;
use framework::prelude::Texture;
use packets::structures::Direction;
use std::sync::Arc;

#[derive(Default, Clone)]
pub struct PlayerForm {
    pub activated: bool,
    pub deactivated: bool,
    pub mug_texture: Option<Arc<Texture>>,
    pub description: Arc<String>,
    pub activate_callback: Option<BattleCallback>,
    pub deactivate_callback: Option<BattleCallback>,
    pub update_callback: Option<BattleCallback>,
    pub calculate_charge_time_callback: Option<BattleCallback<u8, FrameTime>>,
    pub charged_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub special_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub can_charge_card_callback: Option<BattleCallback<CardProperties, bool>>,
    pub charged_card_callback: Option<BattleCallback<CardProperties, Option<GenerationalIndex>>>,
    pub movement_callback: Option<BattleCallback<Direction>>,
}
