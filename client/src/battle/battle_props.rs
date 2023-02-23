use crate::packages::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::BlockGrid;
use crate::saves::Deck;
use framework::prelude::*;
use packets::structures::{BattleStatistics, InstalledBlock};
use std::collections::VecDeque;

pub struct PlayerSetup<'a> {
    pub player_package: &'a PlayerPackage,
    pub health: i32,
    pub base_health: i32,
    pub deck: Deck,
    pub blocks: Vec<InstalledBlock>,
    pub index: usize,
    pub local: bool,
    pub input_buffer: VecDeque<Vec<Input>>,
}

impl<'a> PlayerSetup<'a> {
    pub fn new(player_package: &'a PlayerPackage, index: usize, local: bool) -> Self {
        Self {
            player_package,
            health: 9999,
            base_health: 9999,
            deck: Deck::new(String::new()),
            blocks: Vec::new(),
            index,
            local,
            input_buffer: VecDeque::new(),
        }
    }

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
        let deck = global_save.active_deck().cloned().unwrap_or_default();
        let blocks = global_save.active_blocks().cloned().unwrap_or_default();

        let grid = BlockGrid::new(PackageNamespace::Local).with_blocks(game_io, blocks.clone());

        let health_boost = grid
            .valid_packages(game_io)
            .fold(0, |acc, package| acc + package.health_boost);

        Self {
            player_package,
            health: player_package.health + health_boost,
            base_health: player_package.health,
            index: 0,
            deck,
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
