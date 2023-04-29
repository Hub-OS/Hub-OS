use crate::render::Animator;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::scenes::BattleEvent;
use framework::prelude::{GameIO, Texture};
use std::sync::Arc;

pub struct SharedBattleAssets {
    pub statuses_texture: Arc<Texture>,
    pub statuses_animator: Animator,
    pub event_sender: flume::Sender<BattleEvent>,
    pub attempting_flee: bool,
}

impl SharedBattleAssets {
    pub fn new(game_io: &GameIO, event_sender: flume::Sender<BattleEvent>) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;

        Self {
            statuses_texture: assets.texture(game_io, ResourcePaths::BATTLE_STATUSES),
            statuses_animator: Animator::load_new(assets, ResourcePaths::BATTLE_STATUSES_ANIMATION),
            event_sender,
            attempting_flee: false,
        }
    }
}
