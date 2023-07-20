use super::{AssetManager, Globals};
use crate::packages::*;
use crate::resources::{GlobalMusic, GlobalSfx, LocalAssetManager, ResourcePaths};
use framework::prelude::GameIO;
use packets::structures::FileHash;
use std::collections::HashSet;

pub struct ProgressUpdate {
    pub label: &'static str,
    pub progress: usize,
    pub total: usize,
}

pub enum BootEvent {
    ProgressUpdate(ProgressUpdate),
    Music(GlobalMusic),
    Sfx(Box<GlobalSfx>),
    PlayerManager(PackageManager<PlayerPackage>),
    CardManager(PackageManager<CardPackage>),
    EncounterManager(PackageManager<EncounterPackage>),
    AugmentManager(PackageManager<AugmentPackage>),
    LibraryManager(PackageManager<LibraryPackage>),
    CharacterManager(PackageManager<CharacterPackage>),
    Done,
}

pub struct BootThread {
    sender: flume::Sender<BootEvent>,
    assets: LocalAssetManager,
    child_packages: Vec<ChildPackageInfo>,
    hashes: HashSet<FileHash>,
}

impl BootThread {
    pub fn spawn(game_io: &GameIO) -> flume::Receiver<BootEvent> {
        let (sender, receiver) = flume::unbounded();
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = globals.assets.clone();

        std::thread::spawn(move || {
            let mut context = Self {
                sender,
                assets,
                child_packages: Vec::new(),
                hashes: HashSet::new(),
            };

            context.load_audio();
            context.load_packages();
        });

        receiver
    }

    fn send(&mut self, event: BootEvent) {
        self.sender.send(event).unwrap();
    }

    fn load_audio(&mut self) {
        // total for music, add 1 for sound font
        let total = GlobalMusic::total() + 1;

        // load sound font for music
        self.send(BootEvent::ProgressUpdate(ProgressUpdate {
            label: "Loading Music",
            progress: 0,
            total,
        }));

        let sound_font_bytes = self.assets.binary(ResourcePaths::SOUND_FONT);

        // load music
        let mut progress = 1;

        let load = |path: &str| {
            self.send(BootEvent::ProgressUpdate(ProgressUpdate {
                label: "Loading Music",
                progress,
                total,
            }));

            progress += 1;

            self.assets.non_midi_audio(path)
        };

        let music = GlobalMusic::load_with(sound_font_bytes, load);
        self.send(BootEvent::Music(music));

        // load sfx
        let mut progress = 0;
        let total = GlobalSfx::total();

        let load = |path: &str| {
            self.send(BootEvent::ProgressUpdate(ProgressUpdate {
                label: "Loading SFX",
                progress,
                total,
            }));

            progress += 1;

            self.assets.non_midi_audio(path)
        };

        let sfx = Box::new(GlobalSfx::load_with(load));
        self.send(BootEvent::Sfx(sfx));
    }

    fn load_packages(&mut self) {
        // load players
        let player_packages =
            self.load_package_folder(PackageCategory::Player, "./mods/players", "Loading Players");

        self.send(BootEvent::PlayerManager(player_packages));

        // load cards
        let card_packages =
            self.load_package_folder(PackageCategory::Card, "./mods/cards", "Loading Cards");

        self.send(BootEvent::CardManager(card_packages));

        // load battles
        let encounter_packages = self.load_package_folder(
            PackageCategory::Encounter,
            "./mods/encounters",
            "Loading Battles",
        );

        self.send(BootEvent::EncounterManager(encounter_packages));

        // load augments
        let augment_packages = self.load_package_folder(
            PackageCategory::Augment,
            "./mods/augments",
            "Loading Augments",
        );

        self.send(BootEvent::AugmentManager(augment_packages));

        // load libraries
        let library_packages = self.load_package_folder(
            PackageCategory::Library,
            "./mods/libraries",
            "Loading Libraries",
        );

        self.send(BootEvent::LibraryManager(library_packages));

        // load child packages
        self.load_child_packages();

        // clean cache
        self.clean_cache_folder();

        // complete
        self.send(BootEvent::Done);
    }

    fn load_package_folder<P: Package>(
        &mut self,
        category: PackageCategory,
        folder: &str,
        label: &'static str,
    ) -> PackageManager<P> {
        let mut package_manager = PackageManager::<P>::new(category);
        package_manager.load_packages_in_folder(&self.assets, folder, |progress, total| {
            let status_update = ProgressUpdate {
                label,
                progress,
                total,
            };

            let event = BootEvent::ProgressUpdate(status_update);
            self.sender.send(event).unwrap();
        });

        // gather hashes
        for package in package_manager.packages(PackageNamespace::Local) {
            self.hashes.insert(package.package_info().hash);
        }

        // track child packages
        self.child_packages
            .extend(package_manager.child_packages(PackageNamespace::Local));

        package_manager
    }

    fn load_child_packages(&mut self) {
        let mut character_packages =
            PackageManager::<CharacterPackage>::new(PackageCategory::Character);

        // characters and child packages are currently the same
        let total_child_packages = self.child_packages.len();

        for (i, child_package) in self.child_packages.iter().enumerate() {
            let status_update = ProgressUpdate {
                label: "Loading Enemies",
                progress: i,
                total: total_child_packages,
            };

            let event = BootEvent::ProgressUpdate(status_update);
            self.sender.send(event).unwrap();

            character_packages.load_child_package(PackageNamespace::Local, child_package);
        }

        self.send(BootEvent::CharacterManager(character_packages));
    }

    pub fn clean_cache_folder(&mut self) {
        let Ok(entry_iter) = std::fs::read_dir(ResourcePaths::MOD_CACHE_FOLDER) else {
            return;
        };

        let entries: Vec<_> = entry_iter
            .flatten()
            .filter(|entry| {
                let name = entry.file_name();
                let Some(name) = name.to_str() else {
                    // invalid name, delete
                    return true;
                };

                let Some(hash) = FileHash::from_hex(name.trim_end_matches(".zip")) else {
                    // invalid hash, delete
                    return true;
                };

                !self.hashes.contains(&hash)
            })
            .collect();

        let total = entries.len();

        for (i, entry) in entries.into_iter().enumerate() {
            let status_update = ProgressUpdate {
                label: "Cleaning Cache",
                progress: i,
                total,
            };

            self.send(BootEvent::ProgressUpdate(status_update));

            if let Err(e) = std::fs::remove_file(entry.path()) {
                log::error!("Failed to delete {:?}: {e}", entry.path());
            }
        }
    }
}
