use crate::lua_api::BattleVmManager;
use crate::render::Animator;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::scenes::BattleEvent;
use framework::prelude::{GameIO, Texture};
use std::cell::RefCell;
use std::sync::Arc;

/// Resources that are shared between battle snapshots
pub struct SharedBattleResources {
    pub vm_manager: BattleVmManager,
    pub statuses_texture: Arc<Texture>,
    pub statuses_animator: RefCell<Animator>,
    pub event_sender: flume::Sender<BattleEvent>,
    pub event_receiver: flume::Receiver<BattleEvent>,
}

impl SharedBattleResources {
    pub fn new(game_io: &GameIO) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;

        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            vm_manager: BattleVmManager::new(),
            statuses_texture: assets.texture(game_io, ResourcePaths::BATTLE_STATUSES),
            statuses_animator: RefCell::new(Animator::load_new(
                assets,
                ResourcePaths::BATTLE_STATUSES_ANIMATION,
            )),
            event_sender,
            event_receiver,
        }
    }
}
