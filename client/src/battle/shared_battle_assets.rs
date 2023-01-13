use crate::render::Animator;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::{GameIO, Texture};
use std::sync::Arc;

pub struct SharedBattleAssets {
    pub statuses_texture: Arc<Texture>,
    pub statuses_animator: Animator,
}

impl SharedBattleAssets {
    pub fn new(game_io: &GameIO) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;

        Self {
            statuses_texture: assets.texture(game_io, ResourcePaths::BATTLE_STATUSES),
            statuses_animator: Animator::load_new(assets, ResourcePaths::BATTLE_STATUSES_ANIMATION),
        }
    }
}
