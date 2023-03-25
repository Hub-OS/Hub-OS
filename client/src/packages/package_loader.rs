use std::collections::HashSet;

use super::*;
use crate::resources::{LocalAssetManager, ResourcePaths};
use framework::prelude::GameIO;
use packets::structures::FileHash;

pub struct PackageProgressUpdate {
    pub label: &'static str,
    pub progress: usize,
    pub total: usize,
}

pub enum PackageEvent {
    ProgressUpdate(PackageProgressUpdate),
    PlayerManager(PackageManager<PlayerPackage>),
    CardManager(PackageManager<CardPackage>),
    EncounterManager(PackageManager<EncounterPackage>),
    AugmentManager(PackageManager<AugmentPackage>),
    LibraryManager(PackageManager<LibraryPackage>),
    CharacterManager(PackageManager<CharacterPackage>),
    Done,
}

pub struct PackageLoader {
    sender: flume::Sender<PackageEvent>,
    assets: LocalAssetManager,
    child_packages: Vec<ChildPackageInfo>,
    hashes: HashSet<FileHash>,
}

impl PackageLoader {
    pub fn create_thread(game_io: &GameIO) -> flume::Receiver<PackageEvent> {
        let (sender, receiver) = flume::unbounded();
        let assets = LocalAssetManager::new(game_io);

        std::thread::spawn(move || {
            let mut package_loader = Self {
                sender,
                assets,
                child_packages: Vec::new(),
                hashes: HashSet::new(),
            };

            // load players
            let player_packages = package_loader.load_packages(
                PackageCategory::Player,
                "./mods/players",
                "Loading Players",
            );

            package_loader.send(PackageEvent::PlayerManager(player_packages));

            // load cards
            let card_packages = package_loader.load_packages(
                PackageCategory::Card,
                "./mods/cards",
                "Loading Cards",
            );

            package_loader.send(PackageEvent::CardManager(card_packages));

            // load battles
            let encounter_packages = package_loader.load_packages(
                PackageCategory::Encounter,
                "./mods/encounters",
                "Loading Battles",
            );

            package_loader.send(PackageEvent::EncounterManager(encounter_packages));

            // load augments
            let augment_packages = package_loader.load_packages(
                PackageCategory::Augment,
                "./mods/augments",
                "Loading Augments",
            );

            package_loader.send(PackageEvent::AugmentManager(augment_packages));

            // load libraries
            let library_packages = package_loader.load_packages(
                PackageCategory::Library,
                "./mods/libraries",
                "Loading Libraries",
            );

            package_loader.send(PackageEvent::LibraryManager(library_packages));

            // load child packages
            package_loader.load_child_packages();

            // clean cache
            package_loader.clean_cache_folder();

            // complete
            package_loader.send(PackageEvent::Done);
        });

        receiver
    }

    fn send(&mut self, event: PackageEvent) {
        self.sender.send(event).unwrap();
    }

    fn load_packages<P: Package>(
        &mut self,
        category: PackageCategory,
        folder: &str,
        label: &'static str,
    ) -> PackageManager<P> {
        let mut package_manager = PackageManager::<P>::new(category);
        package_manager.load_packages_in_folder(&self.assets, folder, |progress, total| {
            let status_update = PackageProgressUpdate {
                label,
                progress,
                total,
            };

            let event = PackageEvent::ProgressUpdate(status_update);
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
            let status_update = PackageProgressUpdate {
                label: "Loading Enemies",
                progress: i,
                total: total_child_packages,
            };

            let event = PackageEvent::ProgressUpdate(status_update);
            self.sender.send(event).unwrap();

            character_packages.load_child_package(PackageNamespace::Local, child_package);
        }

        self.send(PackageEvent::CharacterManager(character_packages));
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
            let status_update = PackageProgressUpdate {
                label: "Cleaning Cache",
                progress: i,
                total,
            };

            self.send(PackageEvent::ProgressUpdate(status_update));

            if let Err(e) = std::fs::remove_file(entry.path()) {
                log::error!("Failed to delete {:?}: {e}", entry.path());
            }
        }
    }
}
