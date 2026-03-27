use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::{FontName, LogBox, Text};
use crate::render::*;
use crate::resources::*;
use crate::scenes::MainMenuScene;
use crate::tips::Tip;
use framework::logging::LogRecord;
use framework::prelude::*;
use packets::structures::FileHash;
use std::collections::HashSet;

pub struct ProgressUpdate {
    pub label_translation_key: &'static str,
    pub progress: usize,
    pub total: usize,
}

pub enum Event {
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

/// Visible to the user, loads all mods
pub struct BootStage2 {
    camera: Camera,
    background: Background,
    status_label: Text,
    log_frame_sprite: Sprite,
    progress_bar_sprite: Sprite,
    progress_bar_bounds: Rect,
    status_position: Vec2,
    log_box: LogBox,
    log_receiver: flume::Receiver<LogRecord>,
    event_receiver: flume::Receiver<Event>,
    done: bool,
    next_scene: NextScene,
}

impl BootStage2 {
    pub fn new(game_io: &GameIO, log_receiver: flume::Receiver<LogRecord>) -> BootStage2 {
        Tip::log_random(game_io);

        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        // ui
        let mut animator = Animator::load_new(assets, ResourcePaths::BOOT_UI_ANIMATION);
        animator.set_state("DEFAULT");

        let progress_bar_bounds = Rect::from_corners(
            animator.point_or_zero("BAR_START"),
            animator.point_or_zero("BAR_END"),
        );

        let log_bounds = Rect::from_corners(
            animator.point_or_zero("LOG_START"),
            animator.point_or_zero("LOG_END"),
        );

        let status_position = animator.point_or_zero("STATUS_CENTER");

        // progress bar
        let mut progress_bar_sprite = assets.new_sprite(game_io, ResourcePaths::BOOT_UI);
        animator.set_state("PROGRESS_BAR");
        animator.apply(&mut progress_bar_sprite);

        // status text
        let mut status_label = Text::new(game_io, FontName::Code);
        status_label.style.color = Color::WHITE;
        status_label.style.shadow_color = TEXT_TRANSPARENT_SHADOW_COLOR;

        // logs
        let log_box = LogBox::new(game_io, log_bounds);

        let mut log_frame_sprite = assets.new_sprite(game_io, ResourcePaths::BOOT_UI);
        animator.set_state("LOG_FRAME");
        animator.apply(&mut log_frame_sprite);

        // work thread
        let receiver = BootStage2Thread::spawn(game_io);

        BootStage2 {
            camera: Camera::new_ui(game_io),
            background: Background::new_main_menu(game_io),
            status_label,
            log_frame_sprite,
            progress_bar_sprite,
            progress_bar_bounds,
            status_position,
            log_box,
            log_receiver,
            event_receiver: receiver,
            done: false,
            next_scene: NextScene::None,
        }
    }

    fn handle_thread_messages(&mut self, game_io: &mut GameIO) {
        while let Ok(record) = self.log_receiver.try_recv() {
            let high_priority_internal = record.target.starts_with(env!("CARGO_PKG_NAME"))
                && record.level != log::Level::Trace;

            let high_priority_external = if cfg!(debug_assertions) {
                // display warnings in debug builds
                matches!(record.level, log::Level::Warn | log::Level::Error)
            } else {
                // ignore warnings in release builds
                record.level == log::Level::Error
            };

            if high_priority_internal || high_priority_external {
                self.log_box.push_record(record);
            }
        }

        if self.done {
            return;
        }

        let globals = Globals::from_resources_mut(game_io);

        while let Ok(message) = self.event_receiver.try_recv() {
            match message {
                Event::ProgressUpdate(status_update) => {
                    // update progress text
                    self.status_label.text = globals.translate(status_update.label_translation_key);

                    // update progress bar
                    let multiplier = status_update.progress as f32 / status_update.total as f32;
                    self.update_progress_bar(multiplier);
                }
                Event::Music(music) => {
                    globals.music = music;
                }
                Event::Sfx(sfx) => {
                    globals.sfx = sfx;
                }
                Event::PlayerManager(player_packages) => {
                    globals.player_packages = player_packages;
                }
                Event::CardManager(card_packages) => {
                    globals.card_packages = card_packages;

                    // load recipes
                    let ns = PackageNamespace::Local;
                    for package in globals.card_packages.packages(ns) {
                        globals.card_recipes.load_from_package(ns, package);
                    }
                }
                Event::EncounterManager(encounter_packages) => {
                    globals.encounter_packages = encounter_packages;
                }
                Event::AugmentManager(augment_packages) => {
                    globals.augment_packages = augment_packages;
                }
                Event::StatusManager(status_packages) => {
                    globals.status_packages = status_packages;
                }
                Event::TileStateManager(tile_state_packages) => {
                    globals.tile_state_packages = tile_state_packages;
                }
                Event::LibraryManager(library_packages) => {
                    globals.library_packages = library_packages;
                }
                Event::CharacterManager(character_packages) => {
                    globals.character_packages = character_packages;
                }
                Event::Done => {
                    let message = globals.translate("boot-press-any-button");
                    self.status_label.text = message;
                    self.update_progress_bar(1.0);
                    self.done = true;
                }
            }
        }

        let metrics = self.status_label.measure();
        let status_position = self.status_position - metrics.size * 0.5;
        self.status_label.style.bounds.set_position(status_position);
    }

    fn update_progress_bar(&mut self, multiplier: f32) {
        let mut bounds = self.progress_bar_bounds;

        bounds.width = (bounds.width * multiplier).floor();
        self.progress_bar_sprite.set_bounds(bounds);
    }

    fn transfer(&mut self, game_io: &mut GameIO) {
        let mut scene = MainMenuScene::new(game_io);
        scene.set_background(self.background.clone());
        let transition = crate::transitions::new_boot(game_io);
        self.next_scene = NextScene::new_swap(scene).with_transition(transition);
    }
}

impl Scene for BootStage2 {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        if self.done {
            let scene = MainMenuScene::new(game_io);
            let transition = crate::transitions::new_sub_scene_pop(game_io);
            self.next_scene = NextScene::new_swap(scene).with_transition(transition);
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.handle_thread_messages(game_io);

        if game_io.is_in_transition() {
            return;
        }

        let input_util = InputUtil::new(game_io);

        // transfer to the next scene
        if self.done && input_util.latest_input().is_some() {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.start_game);

            self.transfer(game_io);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw bg
        self.background.draw(game_io, render_pass);

        // draw ui
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw progress bar
        sprite_queue.draw_sprite(&self.log_frame_sprite);
        sprite_queue.draw_sprite(&self.progress_bar_sprite);

        // draw logs
        self.log_box.draw(game_io, &mut sprite_queue);

        // draw status
        self.status_label.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

struct BootStage2Thread {
    sender: flume::Sender<Event>,
    assets: LocalAssetManager,
    child_packages: Vec<(ChildPackageInfo, PackageNamespace)>,
    hashes: HashSet<FileHash>,
}

impl BootStage2Thread {
    fn spawn(game_io: &GameIO) -> flume::Receiver<Event> {
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

            let _ = (move || -> anyhow::Result<()> {
                context.load_audio()?;
                context.load_packages()?;
                Ok(())
            })();
        });

        receiver
    }

    fn load_audio(&mut self) -> anyhow::Result<()> {
        // total for music, add 1 for sound font
        let total = GlobalMusic::total() + 1;

        // load sound font for music
        self.sender.send(Event::ProgressUpdate(ProgressUpdate {
            label_translation_key: "boot-loading-music",
            progress: 0,
            total,
        }))?;

        let sound_font_bytes = self.assets.binary(ResourcePaths::SOUND_FONT);

        // load music
        let mut progress = 1;

        let load = |path: &str| -> Result<Vec<SoundBuffer>, flume::SendError<Event>> {
            self.sender.send(Event::ProgressUpdate(ProgressUpdate {
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
        self.sender.send(Event::Music(music))?;

        // load sfx
        let mut progress = 0;
        let total = GlobalSfx::total();

        let load = |path: &str| -> Result<SoundBuffer, flume::SendError<Event>> {
            self.sender.send(Event::ProgressUpdate(ProgressUpdate {
                label_translation_key: "boot-loading-sfx",
                progress,
                total,
            }))?;

            progress += 1;

            Ok(self.assets.non_midi_audio(path))
        };

        let sfx = Box::new(GlobalSfx::load_with(load)?);
        self.sender.send(Event::Sfx(sfx))?;

        Ok(())
    }

    fn load_packages(&mut self) -> anyhow::Result<()> {
        // load players
        let player_packages =
            self.load_category(PackageCategory::Player, "boot-loading-player-mods")?;

        self.sender.send(Event::PlayerManager(player_packages))?;

        // load cards
        let card_packages = self.load_category(PackageCategory::Card, "boot-loading-card-mods")?;

        self.sender.send(Event::CardManager(card_packages))?;

        // load battles
        let encounter_packages =
            self.load_category(PackageCategory::Encounter, "boot-loading-encounter-mods")?;

        self.sender
            .send(Event::EncounterManager(encounter_packages))?;

        // load augments
        let augment_packages =
            self.load_category(PackageCategory::Augment, "boot-loading-augment-mods")?;

        self.sender.send(Event::AugmentManager(augment_packages))?;

        // load statuses
        let status_packages =
            self.load_category(PackageCategory::Status, "boot-loading-status-mods")?;

        self.sender.send(Event::StatusManager(status_packages))?;

        // load tile states
        let tile_state_packages =
            self.load_category(PackageCategory::TileState, "boot-loading-tile-state-mods")?;

        self.sender
            .send(Event::TileStateManager(tile_state_packages))?;

        // load libraries
        let library_packages =
            self.load_category(PackageCategory::Library, "boot-loading-library-mods")?;

        self.sender.send(Event::LibraryManager(library_packages))?;

        // load child packages
        self.load_child_packages()?;

        // clean cache
        self.clean_cache_folder()?;

        // complete
        self.sender.send(Event::Done)?;

        Ok(())
    }

    fn load_category<P: Package>(
        &mut self,
        category: PackageCategory,
        label_translation_key: &'static str,
    ) -> anyhow::Result<PackageManager<P>> {
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
    ) -> anyhow::Result<()> {
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

                let event = Event::ProgressUpdate(status_update);

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

    fn load_child_packages(&mut self) -> anyhow::Result<()> {
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

            self.sender.send(Event::ProgressUpdate(status_update))?;

            character_packages.load_child_package(*namespace, child_package);
        }

        self.sender
            .send(Event::CharacterManager(character_packages))?;

        Ok(())
    }

    fn clean_cache_folder(&mut self) -> anyhow::Result<()> {
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

            self.sender.send(Event::ProgressUpdate(status_update))?;

            if let Err(e) = std::fs::remove_file(entry.path()) {
                log::error!("Failed to delete {:?}: {e}", entry.path());
            }
        }

        Ok(())
    }
}
