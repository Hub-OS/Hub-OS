use crate::packages::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::Folder;
use framework::prelude::*;
use packets::structures::{BattleStatistics, InstalledBlock};
use std::collections::VecDeque;

pub struct PlayerSetup<'a> {
    pub player_package: &'a PlayerPackage,
    pub folder: Folder,
    pub blocks: Vec<InstalledBlock>,
    pub index: usize,
    pub local: bool,
    pub input_buffer: VecDeque<Vec<Input>>,
}

impl<'a> PlayerSetup<'a> {
    pub fn namespace(&self) -> PackageNamespace {
        if self.local {
            PackageNamespace::Local
        } else {
            PackageNamespace::Remote(self.index)
        }
    }

    pub fn from_globals(game_io: &'a GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;

        let player_package = global_save.player_package(game_io).unwrap();
        let folder = global_save.active_folder().cloned().unwrap_or_default();
        let blocks = global_save.active_blocks().cloned().unwrap_or_default();

        Self {
            player_package,
            index: 0,
            folder,
            blocks,
            local: true,
            input_buffer: VecDeque::new(),
        }
    }
}

pub type BattleStatisticsCallback = Box<dyn FnOnce(Option<BattleStatistics>)>;

pub struct BattleProps<'a> {
    pub battle_package: Option<&'a BattlePackage>,
    pub data: Option<String>,
    pub seed: Option<u64>,
    pub background: Background,
    pub player_setups: Vec<PlayerSetup<'a>>,
    pub senders: Vec<NetplayPacketSender>,
    pub receivers: Vec<(Option<usize>, NetplayPacketReceiver)>,
    pub statistics_callback: Option<BattleStatisticsCallback>,
}

impl<'a> BattleProps<'a> {
    pub fn new_with_defaults(
        game_io: &'a GameIO,
        battle_package: Option<&'a BattlePackage>,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let background_animator = Animator::load_new(assets, ResourcePaths::BATTLE_BG_ANIMATION);
        let background_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_BG);
        let background = Background::new(background_animator, background_sprite);

        Self {
            battle_package,
            data: None,
            seed: None,
            background,
            player_setups: vec![PlayerSetup::from_globals(game_io)],
            senders: Vec::new(),
            receivers: Vec::new(),
            statistics_callback: None,
        }
    }
}
