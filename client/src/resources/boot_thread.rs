use super::{AssetManager, Globals};
use crate::packages::*;
use crate::resources::{GlobalMusic, GlobalSfx, LocalAssetManager, ResourcePaths, SoundBuffer};
use framework::prelude::GameIO;
use packets::structures::FileHash;
use std::collections::HashSet;

pub struct ProgressUpdate {
    pub label_translation_key: &'static str,
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
        let globals = Globals::from_resources(game_io);
        let assets = globals.assets.clone();

        std::thread::spawn(move || {
            let mut context = Self {
                sender,
                assets,
                child_packages: Vec::new(),
                hashes: HashSet::new(),
            };

            let _ = (move || -> Result<(), flume::SendError<BootEvent>> {
                context.load_audio()?;
                context.load_packages()?;
                Ok(())
            })();
        });

        receiver
    }

    fn load_audio(&mut self) -> Result<(), flume::SendError<BootEvent>> {
        // total for music, add 1 for sound font
        let total = GlobalMusic::total() + 1;

        // load sound font for music
        self.sender.send(BootEvent::ProgressUpdate(ProgressUpdate {
            label_translation_key: "boot-loading-music",
            progress: 0,
            total,
        }))?;

        let sound_font_bytes = self.assets.binary(ResourcePaths::SOUND_FONT);

        // load music
        let mut progress = 1;

        let load = |path: &str| -> Result<Vec<SoundBuffer>, flume::SendError<BootEvent>> {
            self.sender.send(BootEvent::ProgressUpdate(ProgressUpdate {
                label_translation_key: "boot-loading-music",
                progress,
                total,
            }))?;

            progress += 1;

            let mut files = Vec::new();

            // load file directly
            let direct_file_path = path.to_string() + ".ogg";

            if std::fs::exists(&direct_file_path).is_ok_and(|v| v) {
                files.push(self.assets.non_midi_audio(&direct_file_path));
            }

            // load all audio in the matching directory without overwriting cached audio from resource packs
            let dir_path = path.to_string() + ResourcePaths::SEPARATOR;

            for dir in std::fs::read_dir(&dir_path).iter_mut().flatten().flatten() {
                let track_path = dir_path.clone() + &dir.file_name().to_string_lossy();
                self.assets.non_midi_audio(&track_path);
            }

            // resolve from loaded audio matching the path
            self.assets.for_each_loaded_audio(|key, sound_buffer| {
                if key.starts_with(&dir_path) {
                    files.push(sound_buffer.clone());
                }
            });

            Ok(files)
        };

        let music = GlobalMusic::load_with(sound_font_bytes, load)?;
        self.sender.send(BootEvent::Music(music))?;

        // load sfx
        let mut progress = 0;
        let total = GlobalSfx::total();

        let load = |path: &str| -> Result<SoundBuffer, flume::SendError<BootEvent>> {
            self.sender.send(BootEvent::ProgressUpdate(ProgressUpdate {
                label_translation_key: "boot-loading-sfx",
                progress,
                total,
            }))?;

            progress += 1;

            Ok(self.assets.non_midi_audio(path))
        };

        let sfx = Box::new(GlobalSfx::load_with(load)?);
        self.sender.send(BootEvent::Sfx(sfx))?;

        Ok(())
    }

    fn load_packages(&mut self) -> Result<(), flume::SendError<BootEvent>> {
        // load players
        let player_packages =
            self.load_category(PackageCategory::Player, "boot-loading-player-mods")?;

        self.sender
            .send(BootEvent::PlayerManager(player_packages))?;

        // load cards
        let card_packages = self.load_category(PackageCategory::Card, "boot-loading-card-mods")?;

        self.sender.send(BootEvent::CardManager(card_packages))?;

        // load battles
        let encounter_packages =
            self.load_category(PackageCategory::Encounter, "boot-loading-encounter-mods")?;

        self.sender
            .send(BootEvent::EncounterManager(encounter_packages))?;

        // load augments
        let augment_packages =
            self.load_category(PackageCategory::Augment, "boot-loading-augment-mods")?;

        self.sender
            .send(BootEvent::AugmentManager(augment_packages))?;

        // load statuses
        let status_packages =
            self.load_category(PackageCategory::Status, "boot-loading-status-mods")?;

        self.sender
            .send(BootEvent::StatusManager(status_packages))?;

        // load tile states
        let tile_state_packages =
            self.load_category(PackageCategory::TileState, "boot-loading-tile-state-mods")?;

        self.sender
            .send(BootEvent::TileStateManager(tile_state_packages))?;

        // load libraries
        let library_packages =
            self.load_category(PackageCategory::Library, "boot-loading-library-mods")?;

        self.sender
            .send(BootEvent::LibraryManager(library_packages))?;

        // load child packages
        self.load_child_packages()?;

        // clean cache
        self.clean_cache_folder()?;

        // complete
        self.sender.send(BootEvent::Done)?;

        Ok(())
    }

    fn load_category<P: Package>(
        &mut self,
        category: PackageCategory,
        label_translation_key: &'static str,
    ) -> Result<PackageManager<P>, flume::SendError<BootEvent>> {
        let mut package_manager = PackageManager::<P>::new(category);

        self.load_package_folder(
            &mut package_manager,
            PackageNamespace::BuiltIn,
            &ResourcePaths::game_folder_absolute(category.built_in_path()),
            label_translation_key,
        )?;

        self.load_package_folder(
            &mut package_manager,
            PackageNamespace::Local,
            &ResourcePaths::data_folder_absolute(category.mod_path()),
            label_translation_key,
        )?;

        Ok(package_manager)
    }

    fn load_package_folder<P: Package>(
        &mut self,
        package_manager: &mut PackageManager<P>,
        namespace: PackageNamespace,
        path: &str,
        label_translation_key: &'static str,
    ) -> Result<(), flume::SendError<BootEvent>> {
        package_manager.load_packages_in_folder(
            &self.assets,
            namespace,
            path,
            |progress, total| {
                let status_update = ProgressUpdate {
                    label_translation_key,
                    progress,
                    total,
                };

                let event = BootEvent::ProgressUpdate(status_update);

                self.sender.send(event)
            },
        )?;

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

        Ok(())
    }

    fn load_child_packages(&mut self) -> Result<(), flume::SendError<BootEvent>> {
        let mut character_packages =
            PackageManager::<CharacterPackage>::new(PackageCategory::Character);

        // characters and child packages are currently the same
        let total_child_packages = self.child_packages.len();

        for (i, (child_package, namespace)) in self.child_packages.iter().enumerate() {
            let status_update = ProgressUpdate {
                label_translation_key: "boot-loading-character-mods",
                progress: i,
                total: total_child_packages,
            };

            self.sender.send(BootEvent::ProgressUpdate(status_update))?;

            character_packages.load_child_package(*namespace, child_package);
        }

        self.sender
            .send(BootEvent::CharacterManager(character_packages))?;

        Ok(())
    }

    pub fn clean_cache_folder(&mut self) -> Result<(), flume::SendError<BootEvent>> {
        let Ok(entry_iter) = std::fs::read_dir(ResourcePaths::mod_cache_folder()) else {
            return Ok(());
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
                label_translation_key: "boot-cleaning-cache",
                progress: i,
                total,
            };

            self.sender.send(BootEvent::ProgressUpdate(status_update))?;

            if let Err(e) = std::fs::remove_file(entry.path()) {
                log::error!("Failed to delete {:?}: {e}", entry.path());
            }
        }

        Ok(())
    }
}
