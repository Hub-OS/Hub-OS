use crate::packages::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::BattleRecording;
use crate::saves::BlockGrid;
use crate::saves::Deck;
use crate::saves::PlayerInputBuffer;
use framework::prelude::*;
use packets::structures::InstalledSwitchDrive;
use packets::structures::{BattleStatistics, Emotion, InstalledBlock};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone)]
pub struct PlayerSetup {
    pub package_id: PackageId,
    pub script_enabled: bool,
    pub health: i32,
    pub base_health: i32,
    pub emotion: Emotion,
    pub deck: Deck,
    pub blocks: Vec<InstalledBlock>,
    pub drives: Vec<InstalledSwitchDrive>,
    pub index: usize,
    pub local: bool,
    pub buffer: PlayerInputBuffer,
}

impl PlayerSetup {
    pub fn new_dummy(player_package: &PlayerPackage, index: usize, local: bool) -> Self {
        Self {
            package_id: player_package.package_info.id.clone(),
            script_enabled: true,
            health: 9999,
            base_health: 9999,
            emotion: Emotion::default(),
            deck: Deck::new(String::new()),
            blocks: Vec::new(),
            drives: Vec::new(),
            index,
            local,
            buffer: PlayerInputBuffer::default(),
        }
    }

    pub fn namespace(&self) -> PackageNamespace {
        PackageNamespace::Netplay(self.index as u8)
    }

    pub fn drives_augment_iter<'a, 'b: 'a>(
        &'a self,
        game_io: &'b GameIO,
    ) -> impl Iterator<Item = &'b AugmentPackage> + 'a {
        let globals = game_io.resource::<Globals>().unwrap();
        let augment_packages = &globals.augment_packages;
        let namespace = self.namespace();

        self.drives.iter().flat_map(move |drive| {
            augment_packages.package_or_fallback(namespace, &drive.package_id)
        })
    }

    pub fn from_globals(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;
        let restrictions = &globals.restrictions;
        let mut deck_restrictions = restrictions.base_deck_restrictions();

        let player_package = global_save.player_package(game_io).unwrap();
        let player_package_info = &player_package.package_info;

        let script_enabled = restrictions.owns_player(&player_package_info.id)
            && restrictions.validate_package_tree(game_io, player_package_info.triplet());

        let ns = PackageNamespace::Local;

        // blocks
        let blocks: Vec<_> = restrictions
            .filter_blocks(game_io, ns, global_save.active_blocks().iter())
            .cloned()
            .collect();

        let drives = global_save.active_drive_parts().to_vec();

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
            package_id: player_package_info.id.clone(),
            script_enabled,
            health: player_package.health + health_boost,
            base_health: player_package.health,
            emotion: Emotion::default(),
            index: 0,
            deck,
            blocks,
            drives,
            local: true,
            buffer: PlayerInputBuffer::new_with_delay(INPUT_DELAY),
        }
    }

    pub fn player_package<'a>(&self, game_io: &'a GameIO) -> Option<&'a PlayerPackage> {
        let globals = game_io.resource::<Globals>().unwrap();

        globals
            .player_packages
            .package_or_fallback(self.namespace(), &self.package_id)
    }
}

pub type BattleStatisticsCallback = Box<dyn FnOnce(Option<BattleStatistics>)>;

pub struct BattleProps {
    pub encounter_package_pair: Option<(PackageNamespace, PackageId)>,
    pub data: Option<String>,
    pub seed: u64,
    pub background: Background,
    pub player_setups: Vec<PlayerSetup>,
    pub senders: Vec<NetplayPacketSender>,
    pub receivers: Vec<(Option<usize>, NetplayPacketReceiver)>,
    pub statistics_callback: Option<BattleStatisticsCallback>,
    pub recording_enabled: bool,
}

impl BattleProps {
    pub fn new_with_defaults(
        game_io: &GameIO,
        encounter_package_pair: Option<(PackageNamespace, PackageId)>,
    ) -> Self {
        let game_run_duration = game_io.frame_start_instant() - game_io.game_start_instant();

        Self {
            encounter_package_pair,
            data: None,
            seed: game_run_duration.as_secs(),
            background: Background::new_battle(game_io),
            player_setups: vec![PlayerSetup::from_globals(game_io)],
            senders: Vec::new(),
            receivers: Vec::new(),
            statistics_callback: None,
            recording_enabled: true,
        }
    }

    pub fn encounter_package<'a>(&self, game_io: &'a GameIO) -> Option<&'a EncounterPackage> {
        let globals = game_io.resource::<Globals>().unwrap();

        let (ns, id) = self.encounter_package_pair.as_ref()?;
        globals.encounter_packages.package_or_fallback(*ns, id)
    }

    pub fn from_recording(game_io: &GameIO, recording: &BattleRecording) -> Self {
        Self {
            encounter_package_pair: recording.encounter_package_pair.clone(),
            data: recording.data.clone(),
            seed: recording.seed,
            background: Background::new_battle(game_io),
            player_setups: recording.player_setups.clone(),
            senders: Vec::new(),
            receivers: Vec::new(),
            statistics_callback: None,
            recording_enabled: false,
        }
    }

    pub fn try_load_recording(&self, game_io: &mut GameIO) -> Option<BattleRecording> {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // load recording
        let encounter_package = self.encounter_package(game_io)?;
        let recording = BattleRecording::load(assets, encounter_package.recording_path.as_ref()?)?;

        // load packages
        let ignored_package_ids = encounter_package.recording_overrides.clone();
        recording.load_packages(game_io, ignored_package_ids);

        Some(recording)
    }
}
