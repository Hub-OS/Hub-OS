use super::{BattleCallback, PlayerOverridables};
use framework::prelude::Texture;
use std::sync::Arc;

#[derive(Default, Clone)]
pub struct PlayerForm {
    pub activated: bool,
    pub deactivated: bool,
    pub mug_texture: Option<Arc<Texture>>,
    pub description: Option<Arc<str>>,
    pub activate_callback: Option<BattleCallback>,
    pub deactivate_callback: Option<BattleCallback>,
    pub update_callback: Option<BattleCallback>,
    pub overridables: PlayerOverridables,
}
