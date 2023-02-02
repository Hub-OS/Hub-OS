use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::{FontStyle, Text, TextStyle};
use crate::render::*;
use crate::resources::*;
use framework::logging::LogRecord;
use framework::prelude::*;

use super::MainMenuScene;

const LOG_MARGIN: f32 = 2.0;

struct StatusUpdate {
    label: &'static str,
    progress: usize,
    total: usize,
}

enum Event {
    StatusUpdate(StatusUpdate),
    PlayerManager(PackageManager<PlayerPackage>),
    CardManager(PackageManager<CardPackage>),
    BattleManager(PackageManager<BattlePackage>),
    BlockManager(PackageManager<BlockPackage>),
    LibraryManager(PackageManager<LibraryPackage>),
    CharacterManager(PackageManager<CharacterPackage>),
    Done,
}

pub struct BootScene {
    camera: Camera,
    status_label: Text,
    log_style: TextStyle,
    log_receiver: flume::Receiver<LogRecord>,
    log_records: Vec<LogRecord>,
    event_receiver: flume::Receiver<Event>,
    done: bool,
    next_scene: NextScene,
}

impl BootScene {
    pub fn new(game_io: &mut GameIO, log_receiver: flume::Receiver<LogRecord>) -> BootScene {
        game_io.graphics_mut().set_clear_color(Color::BLACK);

        // log text style
        let mut log_style = TextStyle::new(game_io, FontStyle::Thin);
        log_style.bounds.width = RESOLUTION_F.x - LOG_MARGIN * 2.0;
        log_style.scale = Vec2::new(0.5, 0.5);

        let (sender, receiver) = flume::unbounded();

        let thread_assets = LocalAssetManager::new(game_io);

        std::thread::spawn(move || {
            // delete cache folder to prevent infinitely growing cache
            let _ = std::fs::remove_dir_all(ResourcePaths::MOD_CACHE_FOLDER);

            let mut child_packages = Vec::new();

            // load player mods
            let mut player_packages = PackageManager::<PlayerPackage>::new(PackageCategory::Player);
            player_packages.load_packages_in_folder(
                &thread_assets,
                "./mods/players",
                |progress, total| {
                    let status_update = StatusUpdate {
                        label: "Loading Players",
                        progress,
                        total,
                    };

                    sender.send(Event::StatusUpdate(status_update)).unwrap();
                },
            );

            child_packages.extend(player_packages.child_packages(PackageNamespace::Local));

            sender.send(Event::PlayerManager(player_packages)).unwrap();

            // load card mods
            let mut card_packages = PackageManager::<CardPackage>::new(PackageCategory::Card);
            card_packages.load_packages_in_folder(
                &thread_assets,
                "./mods/cards",
                |progress, total| {
                    let status_update = StatusUpdate {
                        label: "Loading Cards",
                        progress,
                        total,
                    };

                    sender.send(Event::StatusUpdate(status_update)).unwrap();
                },
            );

            child_packages.extend(card_packages.child_packages(PackageNamespace::Local));

            sender.send(Event::CardManager(card_packages)).unwrap();

            // load battle mods
            let mut battle_packages = PackageManager::<BattlePackage>::new(PackageCategory::Battle);
            battle_packages.load_packages_in_folder(
                &thread_assets,
                "./mods/enemies",
                |progress, total| {
                    let status_update = StatusUpdate {
                        label: "Loading Battles",
                        progress,
                        total,
                    };

                    sender.send(Event::StatusUpdate(status_update)).unwrap();
                },
            );

            child_packages.extend(battle_packages.child_packages(PackageNamespace::Local));

            sender.send(Event::BattleManager(battle_packages)).unwrap();

            // load block mods
            let mut block_packages = PackageManager::<BlockPackage>::new(PackageCategory::Block);
            block_packages.load_packages_in_folder(
                &thread_assets,
                "./mods/blocks",
                |progress, total| {
                    let status_update = StatusUpdate {
                        label: "Loading Blocks",
                        progress,
                        total,
                    };

                    sender.send(Event::StatusUpdate(status_update)).unwrap();
                },
            );

            child_packages.extend(block_packages.child_packages(PackageNamespace::Local));

            sender.send(Event::BlockManager(block_packages)).unwrap();

            // load libraries
            let mut library_packages =
                PackageManager::<LibraryPackage>::new(PackageCategory::Library);
            library_packages.load_packages_in_folder(
                &thread_assets,
                "./mods/libraries",
                |progress, total| {
                    let status_update = StatusUpdate {
                        label: "Loading Libraries",
                        progress,
                        total,
                    };

                    sender.send(Event::StatusUpdate(status_update)).unwrap();
                },
            );

            child_packages.extend(library_packages.child_packages(PackageNamespace::Local));

            sender
                .send(Event::LibraryManager(library_packages))
                .unwrap();

            // load characters
            let mut character_packages =
                PackageManager::<CharacterPackage>::new(PackageCategory::Character);

            // characters and child packages are currently the same
            let total_child_packages = child_packages.len();

            for (i, child_package) in child_packages.into_iter().enumerate() {
                let status_update = StatusUpdate {
                    label: "Loading Enemies",
                    progress: i,
                    total: total_child_packages,
                };

                sender.send(Event::StatusUpdate(status_update)).unwrap();

                character_packages.load_child_package(PackageNamespace::Local, &child_package);
            }

            sender
                .send(Event::CharacterManager(character_packages))
                .unwrap();

            sender.send(Event::Done).unwrap();
        });

        BootScene {
            camera: Camera::new(game_io),
            status_label: Text::new(game_io, FontStyle::Small),
            log_style,
            log_receiver,
            log_records: Vec::new(),
            event_receiver: receiver,
            done: false,
            next_scene: NextScene::None,
        }
    }

    fn handle_thread_messages(&mut self, game_io: &mut GameIO) {
        use framework::logging::LogLevel;

        while let Ok(record) = self.log_receiver.try_recv() {
            let high_priority = matches!(record.level, LogLevel::Warn | LogLevel::Error);

            if record.target.starts_with(env!("CARGO_PKG_NAME")) || high_priority {
                let text_measurement = self.log_style.measure(&record.message);

                let new_records = text_measurement.line_ranges.iter().map(|range| LogRecord {
                    level: record.level,
                    message: record.message[range.clone()].to_string(),
                    target: record.target.clone(),
                });

                self.log_records.extend(new_records);
            }
        }

        if self.done {
            return;
        }

        while let Ok(message) = self.event_receiver.try_recv() {
            match message {
                Event::StatusUpdate(status_update) => {
                    let percentage = status_update.progress * 100 / status_update.total;
                    self.status_label.text = format!("{} {}%", status_update.label, percentage);
                }
                Event::PlayerManager(player_packages) => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    globals.player_packages = player_packages;

                    // make sure there's a playable character available
                    let playable_characters: Vec<_> =
                        globals.player_packages.local_packages().cloned().collect();

                    if !playable_characters.is_empty() {
                        let selected_id = &globals.global_save.selected_character;

                        if !playable_characters.contains(selected_id) {
                            globals.global_save.selected_character = playable_characters[0].clone();
                            globals.global_save.save();
                        }
                    }
                }
                Event::CardManager(card_packages) => {
                    game_io.resource_mut::<Globals>().unwrap().card_packages = card_packages;
                }
                Event::BattleManager(battle_packages) => {
                    game_io.resource_mut::<Globals>().unwrap().battle_packages = battle_packages;
                }
                Event::BlockManager(block_packages) => {
                    game_io.resource_mut::<Globals>().unwrap().block_packages = block_packages;
                }
                Event::LibraryManager(library_packages) => {
                    game_io.resource_mut::<Globals>().unwrap().library_packages = library_packages;
                }
                Event::CharacterManager(character_packages) => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    globals.character_packages = character_packages;
                }
                Event::Done => {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let mut available_players = globals.player_packages.local_packages();

                    if available_players.next().is_some() {
                        self.status_label.text = String::from("BOOT OK. Press Any Button");
                        self.done = true;
                    } else {
                        self.status_label.text = String::from("Player Mod is Required");
                        log::error!("Player Mod missing");
                    }
                }
            }
        }

        (self.status_label.style.bounds).set_position(-RESOLUTION_F * 0.5 + 2.0);
    }
}

impl Scene for BootScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.handle_thread_messages(game_io);

        self.camera.update(game_io);

        if game_io.is_in_transition() {
            return;
        }

        let input_util = InputUtil::new(game_io);

        // transfer to the next scene
        if self.done && input_util.latest_input().is_some() {
            let scene = MainMenuScene::new(game_io);
            let transition = crate::transitions::new_boot(game_io);
            self.next_scene = NextScene::new_swap(scene).with_transition(transition);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw ui
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw logs
        const MAX_HEIGHT: usize = 20;
        let line_height = self.log_style.line_height();

        for (i, record) in self.log_records.iter().rev().enumerate() {
            if i >= MAX_HEIGHT {
                break;
            }

            use framework::logging::LogLevel;

            let color = match record.level {
                LogLevel::Error => Color::RED,
                LogLevel::Warn => Color::YELLOW,
                LogLevel::Trace | LogLevel::Debug => Color::new(0.5, 0.5, 0.5, 1.0),
                _ => Color::WHITE,
            };

            self.log_style.color = color;

            self.log_style.bounds.x = -RESOLUTION_F.x * 0.5 + LOG_MARGIN;
            self.log_style.bounds.y =
                RESOLUTION_F.y * 0.5 - i as f32 * line_height - LOG_MARGIN - line_height;

            self.log_style
                .draw(game_io, &mut sprite_queue, &record.message);
        }

        // draw status
        self.status_label.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
