use super::{BattleScriptContext, BattleSimulation, SharedBattleResources};
use crate::lua_api::encounter_init;
use crate::packages::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::BattleRecording;
use crate::saves::BlockGrid;
use crate::saves::Deck;
use crate::saves::PlayerInputBuffer;
use framework::prelude::*;
use packets::structures::{
    BattleId, BattleStatistics, Emotion, InstalledBlock, InstalledSwitchDrive, MemoryCell,
};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Serialize, Deserialize, Clone)]
pub struct PlayerSetup {
    pub package_id: PackageId,
    pub script_enabled: bool,
    pub health: i32,
    pub base_health: i32,
    pub emotion: Emotion,
    pub deck: Deck,
    pub recipes: Vec<PackageId>,
    pub blocks: Vec<InstalledBlock>,
    pub drives: Vec<InstalledSwitchDrive>,
    pub memories: HashMap<String, MemoryCell>,
    pub index: usize,
    pub local: bool,
    pub connected: bool,
    pub buffer: PlayerInputBuffer,
}

impl PlayerSetup {
    pub fn new_empty(index: usize, local: bool) -> Self {
        Self {
            package_id: Default::default(),
            script_enabled: true,
            health: 0,
            base_health: 0,
            emotion: Default::default(),
            deck: Default::default(),
            recipes: Default::default(),
            blocks: Default::default(),
            drives: Default::default(),
            memories: Default::default(),
            index,
            local,
            connected: true,
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
        let globals = Globals::from_resources(game_io);
        let augment_packages = &globals.augment_packages;
        let namespace = self.namespace();

        self.drives.iter().flat_map(move |drive| {
            augment_packages.package_or_fallback(namespace, &drive.package_id)
        })
    }

    pub fn from_globals(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let global_save = &globals.global_save;
        let restrictions = &globals.restrictions;
        let mut deck_restrictions = restrictions.base_deck_restrictions();

        let player_package = global_save.player_package(game_io).unwrap();
        let player_package_info = &player_package.package_info;

        let script_enabled = restrictions.validate_player(game_io, player_package);

        let ns = PackageNamespace::Local;

        // blocks
        let blocks: Vec<_> = restrictions
            .filter_blocks(game_io, ns, global_save.active_blocks().iter())
            .cloned()
            .collect();

        let grid = BlockGrid::new(ns).with_blocks(game_io, blocks.clone());

        // switch drives
        let drives: Vec<_> = global_save
            .active_drive_parts()
            .iter()
            .filter(|drive| {
                restrictions.validate_package_tree(
                    game_io,
                    (PackageCategory::Augment, ns, drive.package_id.clone()),
                )
            })
            .cloned()
            .collect();

        // combine blocks + drives for augment logic
        let blocks_iter = grid.augments(game_io);
        let drives_iter = drives
            .iter()
            .flat_map(|drive| globals.augment_packages.package(ns, &drive.package_id))
            .map(|augment| (augment, 1));
        let augments: Vec<_> = blocks_iter.chain(drives_iter).collect();

        // resolve health
        let health_boost = augments.iter().fold(0, |acc, &(package, level)| {
            acc + package.health_boost * level as i32
        });

        // deck
        deck_restrictions.apply_augments(
            script_enabled.then_some(player_package),
            augments.into_iter(),
        );

        let mut deck = global_save.active_deck().cloned().unwrap_or_default();
        deck.conform(game_io, ns, &deck_restrictions);

        let recipes = deck.resolve_relevant_recipes(game_io, restrictions, ns);

        // memories
        let memories = global_save
            .memories
            .get(&player_package_info.id)
            .cloned()
            .unwrap_or_default();

        Self {
            package_id: player_package_info.id.clone(),
            script_enabled,
            health: player_package.health + health_boost,
            base_health: player_package.health,
            emotion: Emotion::default(),
            index: 0,
            deck,
            recipes,
            blocks,
            drives,
            memories,
            local: true,
            connected: true,
            buffer: PlayerInputBuffer::new_with_delay(globals.config.input_delay as _),
        }
    }

    pub fn player_package<'a>(&self, game_io: &'a GameIO) -> Option<&'a PlayerPackage> {
        let globals = Globals::from_resources(game_io);

        globals
            .player_packages
            .package_or_fallback(self.namespace(), &self.package_id)
    }

    pub fn direct_dependency_triplets<'a>(
        &'a self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = (PackageCategory, PackageNamespace, PackageId)> + 'a {
        let ns = self.namespace();

        let player_triplet = (PackageCategory::Player, ns, self.package_id.clone());

        let card_triplet_iter = self
            .deck
            .cards
            .iter()
            .map(move |card| (PackageCategory::Card, ns, card.package_id.clone()));

        let recipe_triplet_iter = self
            .recipes
            .iter()
            .map(move |id| (PackageCategory::Card, ns, id.clone()));

        let block_triplet_iter = BlockGrid::new(ns)
            .with_blocks(game_io, self.blocks.clone())
            .augments(game_io)
            .map(move |(augment, _)| {
                (
                    PackageCategory::Augment,
                    ns,
                    augment.package_info.id.clone(),
                )
            });

        let drive_triplet_iter = self.drives_augment_iter(game_io).map(move |augment| {
            (
                PackageCategory::Augment,
                ns,
                augment.package_info.id.clone(),
            )
        });

        std::iter::once(player_triplet)
            .chain(card_triplet_iter)
            .chain(recipe_triplet_iter)
            .chain(block_triplet_iter)
            .chain(drive_triplet_iter)
    }
}

pub type BattleStatisticsCallback = Box<dyn FnOnce(Option<BattleStatistics>)>;

pub struct BattleMeta {
    pub encounter_package_pair: Option<(PackageNamespace, PackageId)>,
    pub data: Option<String>,
    pub seed: u64,
    pub background: Background,
    pub player_setups: Vec<PlayerSetup>,
    pub player_count: usize,
    pub recording_enabled: bool,
}

impl BattleMeta {
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
            player_count: 1,
            recording_enabled: true,
        }
    }

    // returns package info, and the namespace the package should be loaded with
    pub fn encounter_dependencies<'a>(
        &self,
        game_io: &'a GameIO,
    ) -> Vec<(&'a PackageInfo, PackageNamespace)> {
        let globals = Globals::from_resources(game_io);

        let encounter_triplet_iter = std::iter::once(self.encounter_package(game_io))
            .flatten()
            .map(|package| package.package_info().triplet());

        let status_triplet_iter = globals
            .status_packages
            .packages(PackageNamespace::BuiltIn)
            .map(|package| package.package_info.triplet());

        let tile_state_triplet_iter = globals
            .tile_state_packages
            .packages(PackageNamespace::BuiltIn)
            .map(|package| package.package_info.triplet());

        let mut built_in_dependencies = status_triplet_iter
            .chain(tile_state_triplet_iter)
            .collect::<Vec<_>>();

        // maintain order to avoid desyncs
        built_in_dependencies.sort_by_cached_key(|(_, _, id)| id.clone());

        let triplet_iter = encounter_triplet_iter.chain(built_in_dependencies);

        globals.package_dependency_iter(triplet_iter)
    }

    // returns package info, and the namespace the package should be loaded with
    pub fn player_dependencies<'a>(
        &self,
        game_io: &'a GameIO,
    ) -> Vec<(&'a PackageInfo, PackageNamespace)> {
        let globals = Globals::from_resources(game_io);

        let triplet_iter = self
            .player_setups
            .iter()
            .flat_map(|setup| setup.direct_dependency_triplets(game_io));

        globals.package_dependency_iter(triplet_iter)
    }

    pub fn encounter_package<'a>(&self, game_io: &'a GameIO) -> Option<&'a EncounterPackage> {
        let globals = Globals::from_resources(game_io);

        let (ns, id) = self.encounter_package_pair.as_ref()?;
        globals.encounter_packages.package_or_fallback(*ns, id)
    }

    pub fn from_recording(game_io: &GameIO, recording: BattleRecording) -> Self {
        Self {
            encounter_package_pair: recording.encounter_package_pair.clone(),
            data: recording.data,
            seed: recording.seed,
            background: Background::new_battle(game_io),
            player_count: recording.player_setups.len(),
            player_setups: recording.player_setups,
            recording_enabled: false,
        }
    }

    pub fn try_load_recording(&self, game_io: &mut GameIO) -> Option<BattleRecording> {
        let globals = Globals::from_resources(game_io);
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

#[derive(Default)]
pub struct BattleComms {
    pub senders: Vec<NetplayPacketSender>,
    pub receivers: Vec<(Option<usize>, NetplayPacketReceiver)>,
    pub remote_id: BattleId,
    pub server: Option<(ClientPacketSender, flume::Receiver<(BattleId, String)>)>,
}

pub struct BattleProps {
    pub simulation_and_resources: Option<(BattleSimulation, SharedBattleResources)>,
    pub meta: BattleMeta,
    pub comms: BattleComms,
    pub statistics_callback: Option<BattleStatisticsCallback>,
}

impl BattleProps {
    pub fn new_with_defaults(
        game_io: &GameIO,
        encounter_package_pair: Option<(PackageNamespace, PackageId)>,
    ) -> Self {
        Self {
            simulation_and_resources: None,
            meta: BattleMeta::new_with_defaults(game_io, encounter_package_pair),
            comms: BattleComms::default(),
            statistics_callback: None,
        }
    }

    pub fn load_encounter(&mut self, game_io: &GameIO) {
        // create initial simulation
        let mut simulation = BattleSimulation::new(game_io, &self.meta);

        let mut resources = SharedBattleResources::new(game_io, &self.meta);
        resources.load_encounter_vms(game_io, &mut simulation, &self.meta);

        // load battle package
        if let Some(encounter_package) = self.meta.encounter_package(game_io) {
            let vm_manager = &mut resources.vm_manager;
            let vm_index = vm_manager
                .find_vm_from_info(encounter_package.package_info())
                .unwrap();

            let context = BattleScriptContext {
                vm_index,
                game_io,
                resources: &resources,
                simulation: &mut simulation,
            };

            encounter_init(context, self.meta.data.as_deref());
        }

        let entities = &mut simulation.entities;
        simulation.field.initialize_uninitialized(entities);

        self.simulation_and_resources = Some((simulation, resources));
    }
}
