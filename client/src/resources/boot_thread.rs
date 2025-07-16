use super::{AssetManager, Globals};
use crate::packages::*;
use crate::resources::{GlobalMusic, GlobalSfx, LocalAssetManager, ResourcePaths};
use framework::prelude::GameIO;
use packets::structures::FileHash;
use std::collections::HashSet;
use std::sync::Arc;

pub struct ProgressUpdate {
    pub label: Arc<str>,
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
    StatusManager(PackageManager<StatusPackage>),
    TileStateManager(PackageManager<TileStatePackage>),
    LibraryManager(PackageManager<LibraryPackage>),
    CharacterManager(PackageManager<CharacterPackage>),
    Done,
}

pub struct BootThread {
    sender: flume::Sender<BootEvent>,
    assets: LocalAssetManager,
    child_packages: Vec<(ChildPackageInfo, PackageNamespace)>,
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
            label: Arc::from("Loading Music"),
            progress: 0,
            total,
        }));

        let sound_font_bytes = self.assets.binary(ResourcePaths::SOUND_FONT);

        // load music
        let mut progress = 1;

        let load = |path: &str| {
            self.send(BootEvent::ProgressUpdate(ProgressUpdate {
                label: Arc::from("Loading Music"),
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
                label: Arc::from("Loading SFX"),
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
        let player_packages = self.load_category(PackageCategory::Player, "Players");

        self.send(BootEvent::PlayerManager(player_packages));

        // load cards
        let card_packages = self.load_category(PackageCategory::Card, "Chips");

        self.send(BootEvent::CardManager(card_packages));

        // load battles
        let encounter_packages = self.load_category(PackageCategory::Encounter, "Battles");

        self.send(BootEvent::EncounterManager(encounter_packages));

        // load augments
        let augment_packages = self.load_category(PackageCategory::Augment, "Augments");

        self.send(BootEvent::AugmentManager(augment_packages));

        // load statuses
        let status_packages = self.load_category(PackageCategory::Status, "Statuses");

        self.send(BootEvent::StatusManager(status_packages));

        // load tile states
        let tile_state_packages = self.load_category(PackageCategory::TileState, "Tiles");

        self.send(BootEvent::TileStateManager(tile_state_packages));

        // load libraries
        let library_packages = self.load_category(PackageCategory::Library, "Libraries");

        self.send(BootEvent::LibraryManager(library_packages));

        // load child packages
        self.load_child_packages();

        // clean cache
        self.clean_cache_folder();

        // complete
        self.send(BootEvent::Done);
    }

    fn load_category<P: Package>(
        &mut self,
        category: PackageCategory,
        plural_name: &'static str,
    ) -> PackageManager<P> {
        let mut package_manager = PackageManager::<P>::new(category);

        let label: Arc<str> = Arc::from(format!("Loading {plural_name}"));

        self.load_package_folder(
            &mut package_manager,
            PackageNamespace::BuiltIn,
            &ResourcePaths::game_folder_absolute(category.built_in_path()),
            label.clone(),
        );

        self.load_package_folder(
            &mut package_manager,
            PackageNamespace::Local,
            &ResourcePaths::data_folder_absolute(category.mod_path()),
            label,
        );

        package_manager
    }

    fn load_package_folder<P: Package>(
        &mut self,
        package_manager: &mut PackageManager<P>,
        namespace: PackageNamespace,
        path: &str,
        label: Arc<str>,
    ) {
        package_manager.load_packages_in_folder(
            &self.assets,
            namespace,
            path,
            |progress, total| {
                let status_update = ProgressUpdate {
                    label: label.clone(),
                    progress,
                    total,
                };

                let event = BootEvent::ProgressUpdate(status_update);
                self.sender.send(event).unwrap();
            },
        );

        // gather hashes
        for package in package_manager.packages(namespace) {
            self.hashes.insert(package.package_info().hash);
        }

        // track child packages
        let child_package_iter = package_manager
            .child_packages(namespace)
            .into_iter()
            .map(move |info| (info, namespace));

        self.child_packages.extend(child_package_iter);
    }

    fn load_child_packages(&mut self) {
        let mut character_packages =
            PackageManager::<CharacterPackage>::new(PackageCategory::Character);

        // characters and child packages are currently the same
        let total_child_packages = self.child_packages.len();

        for (i, (child_package, namespace)) in self.child_packages.iter().enumerate() {
            let status_update = ProgressUpdate {
                label: Arc::from("Loading Enemies"),
                progress: i,
                total: total_child_packages,
            };

            let event = BootEvent::ProgressUpdate(status_update);
            self.sender.send(event).unwrap();

            character_packages.load_child_package(*namespace, child_package);
        }

        self.send(BootEvent::CharacterManager(character_packages));
    }

    pub fn clean_cache_folder(&mut self) {
        let Ok(entry_iter) = std::fs::read_dir(ResourcePaths::mod_cache_folder()) else {
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
                label: Arc::from("Cleaning Cache"),
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
