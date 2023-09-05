use crate::packages::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::BlockGrid;
use crate::saves::Deck;
use framework::prelude::*;
use packets::structures::InstalledSwitchDrive;
use packets::structures::{BattleStatistics, Emotion, InstalledBlock};
use packets::NetplayBufferItem;
use std::collections::VecDeque;

pub struct PlayerSetup<'a> {
    pub player_package: &'a PlayerPackage,
    pub script_enabled: bool,
    pub health: i32,
    pub base_health: i32,
    pub emotion: Emotion,
    pub deck: Deck,
    pub blocks: Vec<InstalledBlock>,
    pub drives: Vec<InstalledSwitchDrive>,
    pub index: usize,
    pub local: bool,
    pub buffer: VecDeque<NetplayBufferItem>,
}

impl<'a> PlayerSetup<'a> {
    pub fn new(player_package: &'a PlayerPackage, index: usize, local: bool) -> Self {
        Self {
            player_package,
            script_enabled: true,
            health: 9999,
            base_health: 9999,
            emotion: Emotion::default(),
            deck: Deck::new(String::new()),
            blocks: Vec::new(),
            drives: Vec::new(),
            index,
            local,
            buffer: VecDeque::new(),
        }
    }

    pub fn namespace(&self) -> PackageNamespace {
        PackageNamespace::Netplay(self.index)
    }

    pub fn drives_augment_iter(
        &self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = &'a AugmentPackage> + '_ {
        let globals = game_io.resource::<Globals>().unwrap();
        let augment_packages = &globals.augment_packages;
        let namespace = self.namespace();

        self.drives.iter().flat_map(move |drive| {
            augment_packages.package_or_override(namespace, &drive.package_id)
        })
    }

    pub fn from_globals(game_io: &'a GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;
        let restrictions = &globals.restrictions;
        let mut deck_restrictions = restrictions.base_deck_restrictions();

        let player_package = global_save.player_package(game_io).unwrap();

        let script_enabled = restrictions.owns_player(&player_package.package_info.id)
            && restrictions.validate_package_tree(game_io, player_package.package_info.triplet());

        let ns = PackageNamespace::Local;

        // blocks
        let blocks: Vec<_> = if let Some(blocks) = global_save.active_blocks() {
            restrictions
                .filter_blocks(game_io, ns, blocks.iter())
                .cloned()
                .collect()
        } else {
            Vec::new()
        };

        let drives: Vec<_> = if let Some(drives) = global_save.active_drive_parts() {
            drives.clone()
        } else {
            Vec::new()
        };

        let grid = BlockGrid::new(ns).with_blocks(game_io, blocks.clone());

        // resolve health
        let health_boost = grid.augments(game_io).fold(0, |acc, (package, level)| {
            acc + package.health_boost * level as i32
        });

        // deck
        deck_restrictions.apply_augments(grid.augments(game_io));

        // TODO: Filter Switch Drive restrictions to deck as well.

        let mut deck = global_save.active_deck().cloned().unwrap_or_default();
        deck.conform(game_io, PackageNamespace::Local, &deck_restrictions);

        Self {
            player_package,
            script_enabled,
            health: player_package.health + health_boost,
            base_health: player_package.health,
            emotion: Emotion::default(),
            index: 0,
            deck,
            blocks,
            drives,
            local: true,
            buffer: VecDeque::new(),
        }
    }
}

pub type BattleStatisticsCallback = Box<dyn FnOnce(Option<BattleStatistics>)>;

pub struct BattleProps<'a> {
    pub encounter_package: Option<&'a EncounterPackage>,
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
        encounter_package: Option<&'a EncounterPackage>,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let background_animator = Animator::load_new(assets, ResourcePaths::BATTLE_BG_ANIMATION);
        let background_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_BG);
        let background = Background::new(background_animator, background_sprite);

        Self {
            encounter_package,
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
