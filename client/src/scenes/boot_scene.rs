use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::{FontStyle, LogBox, Text};
use crate::render::*;
use crate::resources::*;
use framework::logging::{LogLevel, LogRecord};
use framework::prelude::*;

use super::{CategoryFilter, MainMenuScene, PackagesScene};

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
    background: Background,
    status_label: Text,
    progress_frame_sprite: Sprite,
    progress_bar_sprite: Sprite,
    progress_bar_bounds: Rect,
    status_position: Vec2,
    log_box: LogBox,
    log_receiver: flume::Receiver<LogRecord>,
    event_receiver: flume::Receiver<Event>,
    done: bool,
    next_scene: NextScene,
}

impl BootScene {
    pub fn new(game_io: &mut GameIO, log_receiver: flume::Receiver<LogRecord>) -> BootScene {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // ui
        let mut animator = Animator::load_new(assets, ResourcePaths::BOOT_UI_ANIMATION);

        // frame
        let mut progress_frame_sprite = assets.new_sprite(game_io, ResourcePaths::BOOT_UI);
        animator.set_state("PROGRESS_FRAME");
        animator.apply(&mut progress_frame_sprite);

        let progress_bar_bounds = Rect::from_corners(
            animator.point("BAR_START").unwrap_or_default(),
            animator.point("BAR_END").unwrap_or_default(),
        ) - animator.origin();

        let log_bounds = Rect::from_corners(
            animator.point("LOG_START").unwrap_or_default(),
            animator.point("LOG_END").unwrap_or_default(),
        ) - animator.origin();

        let status_position = progress_bar_bounds.center();

        // progress bar
        let mut progress_bar_sprite = progress_frame_sprite.clone();
        animator.set_state("PROGRESS_BAR");
        animator.apply(&mut progress_bar_sprite);

        // status text
        let mut status_label = Text::new(game_io, FontStyle::Wide);
        status_label.style.color = Color::new(0.28, 0.65, 0.94, 1.0);
        status_label.style.shadow_color = TEXT_TRANSPARENT_SHADOW_COLOR;

        // logs
        let log_box = LogBox::new(game_io, log_bounds);

        // work thread
        let (sender, receiver) = flume::unbounded();
        Self::create_thread(game_io, sender);

        BootScene {
            camera: Camera::new_ui(game_io),
            background: Background::load_static(game_io, ResourcePaths::BOOT_BG),
            status_label,
            progress_frame_sprite,
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

    fn create_thread(game_io: &GameIO, sender: flume::Sender<Event>) {
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
    }

    fn handle_thread_messages(&mut self, game_io: &mut GameIO) {
        while let Ok(record) = self.log_receiver.try_recv() {
            let high_priority = matches!(record.level, LogLevel::Warn | LogLevel::Error);

            if record.target.starts_with(env!("CARGO_PKG_NAME")) || high_priority {
                self.log_box.push_record(record);
            }
        }

        if self.done {
            return;
        }

        while let Ok(message) = self.event_receiver.try_recv() {
            match message {
                Event::StatusUpdate(status_update) => {
                    // update progress text
                    self.status_label.text = status_update.label.to_string();

                    // update progress bar
                    let multiplier = status_update.progress as f32 / status_update.total as f32;
                    self.update_progress_bar(multiplier);
                }
                Event::PlayerManager(player_packages) => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    globals.player_packages = player_packages;

                    // make sure there's a playable character available
                    let playable_characters: Vec<_> = globals
                        .player_packages
                        .package_ids(PackageNamespace::Local)
                        .cloned()
                        .collect();

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
                    let mut available_players =
                        globals.player_packages.package_ids(PackageNamespace::Local);

                    if available_players.next().is_none() {
                        log::info!("Missing player mod");
                    }

                    let message = "Press Any Button";
                    self.status_label.text = String::from(message);
                    self.update_progress_bar(1.0);
                    self.done = true;
                }
            }
        }

        if !self.status_label.style.font_style.has_lower_case() {
            self.status_label.text = self.status_label.text.to_uppercase();
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
        let has_playable_character = {
            let globals = game_io.resource::<Globals>().unwrap();
            let mut available_players =
                globals.player_packages.package_ids(PackageNamespace::Local);

            available_players.next().is_some()
        };

        if has_playable_character {
            let scene = MainMenuScene::new(game_io);
            let transition = crate::transitions::new_boot(game_io);
            self.next_scene = NextScene::new_swap(scene).with_transition(transition);
        } else {
            let scene = PackagesScene::new(game_io, CategoryFilter::Players);
            let transition = crate::transitions::new_sub_scene(game_io);
            self.next_scene = NextScene::new_push(scene).with_transition(transition);
        }
    }
}

impl Scene for BootScene {
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
        self.handle_thread_messages(game_io);

        if game_io.is_in_transition() {
            return;
        }

        let input_util = InputUtil::new(game_io);

        // transfer to the next scene
        if self.done && input_util.latest_input().is_some() {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.start_game_sfx);

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
        sprite_queue.draw_sprite(&self.progress_frame_sprite);
        sprite_queue.draw_sprite(&self.progress_bar_sprite);

        // draw logs
        self.log_box.draw(game_io, &mut sprite_queue);

        // draw status
        self.status_label.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
