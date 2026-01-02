use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::{FontName, LogBox, Text};
use crate::render::*;
use crate::resources::*;
use crate::tips::Tip;
use framework::logging::LogRecord;
use framework::prelude::*;

use super::MainMenuScene;

const LOG_MARGIN: f32 = 2.0;

pub struct BootScene {
    camera: Camera,
    background: Background,
    status_label: Text,
    log_frame_sprite: Sprite,
    progress_bar_sprite: Sprite,
    progress_bar_bounds: Rect,
    status_position: Vec2,
    log_box: LogBox,
    log_receiver: flume::Receiver<LogRecord>,
    event_receiver: flume::Receiver<BootEvent>,
    done: bool,
    next_scene: NextScene,
}

impl BootScene {
    pub fn new(game_io: &GameIO, log_receiver: flume::Receiver<LogRecord>) -> BootScene {
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
        let receiver = BootThread::spawn(game_io);

        BootScene {
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
                BootEvent::ProgressUpdate(status_update) => {
                    // update progress text
                    self.status_label.text = globals.translate(status_update.label_translation_key);

                    // update progress bar
                    let multiplier = status_update.progress as f32 / status_update.total as f32;
                    self.update_progress_bar(multiplier);
                }
                BootEvent::Music(music) => {
                    globals.music = music;
                }
                BootEvent::Sfx(sfx) => {
                    globals.sfx = sfx;
                }
                BootEvent::PlayerManager(player_packages) => {
                    globals.player_packages = player_packages;
                }
                BootEvent::CardManager(card_packages) => {
                    globals.card_packages = card_packages;

                    // load recipes
                    let ns = PackageNamespace::Local;
                    for package in globals.card_packages.packages(ns) {
                        globals.card_recipes.load_from_package(ns, package);
                    }
                }
                BootEvent::EncounterManager(encounter_packages) => {
                    globals.encounter_packages = encounter_packages;
                }
                BootEvent::AugmentManager(augment_packages) => {
                    globals.augment_packages = augment_packages;
                }
                BootEvent::StatusManager(status_packages) => {
                    globals.status_packages = status_packages;
                }
                BootEvent::TileStateManager(tile_state_packages) => {
                    globals.tile_state_packages = tile_state_packages;
                }
                BootEvent::LibraryManager(library_packages) => {
                    globals.library_packages = library_packages;
                }
                BootEvent::CharacterManager(character_packages) => {
                    globals.character_packages = character_packages;
                }
                BootEvent::Done => {
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
