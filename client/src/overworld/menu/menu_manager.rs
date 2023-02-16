use super::Bbs;
use crate::ease::inverse_lerp;
use crate::overworld::OverworldPlayerData;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::transitions::DEFAULT_FADE_DURATION;
use framework::prelude::NextScene;
use framework::prelude::{Color, GameIO, Sprite};

enum Event {
    SelectionMade,
    SelectionAck,
    BbsReplaced,
    BbsClosed { transition: bool },
}

pub trait Menu {
    fn is_fullscreen(&self) -> bool;
    fn is_open(&self) -> bool;
    fn open(&mut self);
    fn handle_input(&mut self, game_io: &mut GameIO);
    fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue);
}

pub struct MenuManager {
    selection_needs_ack: bool,
    textbox: Textbox,
    bbs_replaced: bool,
    old_bbs: Option<Bbs>,
    bbs: Option<Bbs>,
    fade_time: FrameTime,
    max_fade_time: FrameTime,
    fade_sprite: Sprite,
    navigation_menu: NavigationMenu,
    menus: Vec<Box<dyn Menu>>,
    menu_bindings: Vec<(Input, usize)>,
    active_menu: Option<usize>,
    event_receiver: flume::Receiver<Event>,
    event_sender: flume::Sender<Event>,
}

impl MenuManager {
    pub fn new(game_io: &GameIO) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;
        let mut fade_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        fade_sprite.set_color(Color::BLACK);
        fade_sprite.set_size(RESOLUTION_F);

        let max_fade_time =
            (DEFAULT_FADE_DURATION.as_secs_f32() * game_io.target_fps() as f32) as FrameTime;

        let (event_sender, event_receiver) = flume::unbounded();

        let navigation_menu = NavigationMenu::new(
            game_io,
            vec![
                SceneOption::Folders,
                // SceneOption::Items,
                SceneOption::Library,
                SceneOption::Character,
                // SceneOption::Email,
                // SceneOption::KeyItems,
                SceneOption::BattleSelect,
                SceneOption::Config,
            ],
        )
        .into_overlay(true);

        Self {
            selection_needs_ack: false,
            textbox: Textbox::new_overworld(game_io),
            bbs_replaced: false,
            old_bbs: None,
            bbs: None,
            fade_time: max_fade_time,
            max_fade_time,
            fade_sprite,
            navigation_menu,
            menus: Vec::new(),
            menu_bindings: Vec::new(),
            active_menu: None,
            event_sender,
            event_receiver,
        }
    }

    pub fn is_open(&self) -> bool {
        self.active_menu.is_some()
            || self.textbox.is_open()
            || self.bbs.is_some()
            || self.navigation_menu.is_open()
    }

    pub fn is_blocking_hud(&self) -> bool {
        self.navigation_menu.is_open()
    }

    pub fn bbs_mut(&mut self) -> Option<&mut Bbs> {
        self.bbs.as_mut()
    }

    pub fn register_menu(&mut self, menu: Box<dyn Menu>) -> usize {
        self.menus.push(menu);
        self.menus.len() - 1
    }

    /// Binds an input event to open a menu, avoids opening menus while other menus are open
    pub fn bind_menu(&mut self, input: Input, menu_index: usize) {
        self.menu_bindings.push((input, menu_index));
    }

    /// Opens a registered menu, forces other registered menus to close (built in menus such as Textbox + BBS are excluded)
    pub fn open_menu(&mut self, menu_index: usize) {
        self.active_menu = Some(menu_index);
    }

    pub fn acknowledge_selection(&mut self) {
        // sending an event in case the SelectionMade event is still queued
        self.event_sender.send(Event::SelectionAck).unwrap();
    }

    pub fn open_bbs(
        &mut self,
        game_io: &GameIO,
        topic: String,
        color: Color,
        transition: bool,
        on_select: impl Fn(&str) + 'static,
        on_close: impl Fn() + 'static,
    ) {
        if let Some(bbs) = &mut self.bbs {
            self.event_sender.send(Event::BbsReplaced).unwrap();
            bbs.close();
        }

        let on_select = {
            let event_sender = self.event_sender.clone();

            move |id: &str| {
                event_sender.send(Event::SelectionMade).unwrap();
                on_select(id);
            }
        };

        let on_close = {
            let event_sender = self.event_sender.clone();

            move || {
                event_sender.send(Event::BbsClosed { transition }).unwrap();
                on_close();
            }
        };

        if transition {
            self.fade_time = 0;
            self.old_bbs = self.bbs.take();
        } else {
            self.old_bbs = None;
        }

        self.bbs = Some(Bbs::new(game_io, topic, color, on_select, on_close));
    }

    pub fn set_next_avatar(
        &mut self,
        game_io: &GameIO,
        assets: &impl AssetManager,
        texture_path: &str,
        animation_path: &str,
    ) {
        self.textbox
            .set_next_avatar(game_io, assets, texture_path, animation_path);
    }

    pub fn use_player_avatar(&mut self, game_io: &GameIO) {
        self.textbox.use_player_avatar(game_io);
    }

    pub fn remaining_textbox_interfaces(&self) -> usize {
        self.textbox.remaining_interfaces()
    }

    pub fn push_textbox_interface(&mut self, interface: impl TextboxInterface + 'static) {
        self.textbox.push_interface(interface);
        self.textbox.open();
    }

    pub fn update_player_data(&mut self, player_data: &OverworldPlayerData) {
        self.navigation_menu.update_info(player_data);
    }

    pub fn update(&mut self, game_io: &mut GameIO) -> NextScene {
        if self.fade_time < self.max_fade_time {
            self.fade_time += 1;

            let fade_progress = inverse_lerp!(0, self.max_fade_time, self.fade_time);

            if fade_progress > 0.5 {
                // clear out the previous bbs to avoid extra updates
                self.old_bbs = None;
            }
        }

        // handle events
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::SelectionMade => {
                    self.selection_needs_ack = true;
                }
                Event::SelectionAck => {
                    self.selection_needs_ack = false;
                }
                Event::BbsReplaced => {
                    self.bbs_replaced = true;
                }
                Event::BbsClosed { transition } => {
                    if !self.bbs_replaced {
                        // if the bbs was replaced transition will be handled by the replacer
                        // otherwise we must handle it here

                        if transition {
                            self.old_bbs = self.bbs.take();
                            self.fade_time = 0;
                        } else {
                            self.bbs = None;
                            self.old_bbs = None;
                        }
                    }

                    self.bbs_replaced = false;
                }
            }
        }

        let mut handle_input = !self.navigation_menu.is_open();

        // update the active registered menu
        if let Some(index) = self.active_menu {
            let menu = &mut self.menus[index];

            if menu.is_open() {
                menu.handle_input(game_io);

                // skip other input checks while a registered menu is open
                handle_input = false;
            } else {
                self.active_menu = None;
            }
        }

        // update textbox
        if self.textbox.is_complete() {
            self.textbox.close();
        }

        if handle_input {
            self.textbox.update(game_io);
        }

        if self.textbox.is_open() {
            // skip other input checks while a textbox is open
            handle_input = false;
        }

        // update bbs
        if let Some(bbs) = &mut self.bbs {
            if handle_input && self.fade_time == self.max_fade_time && !self.selection_needs_ack {
                bbs.handle_input(game_io);
            }

            bbs.update();

            // skip other input checks while a bbs is open
            handle_input = false;
        }

        if let Some(bbs) = &mut self.old_bbs {
            bbs.update();

            // skip other input checks while a bbs is open
            handle_input = false;
        }

        // try opening a menu if there's no menu open
        if !self.is_open() {
            let input_util = InputUtil::new(game_io);

            for (input, index) in self.menu_bindings.iter().cloned() {
                if input_util.was_just_pressed(input) {
                    self.active_menu = Some(index);
                    self.menus[index].open();

                    // skip other input checks while a menu is opening
                    handle_input = false;
                    break;
                }
            }

            if handle_input && input_util.was_just_pressed(Input::Pause) {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.card_select_open_sfx);
                self.navigation_menu.open();
            }
        }

        self.navigation_menu.update(game_io)
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let fade_progress = inverse_lerp!(0, self.max_fade_time, self.fade_time);

        if fade_progress < 0.5 {
            if let Some(bbs) = &mut self.old_bbs {
                bbs.draw(game_io, sprite_queue);
            }
        } else if let Some(bbs) = &mut self.bbs {
            bbs.draw(game_io, sprite_queue);
        }

        self.textbox.draw(game_io, sprite_queue);

        if self.fade_time < self.max_fade_time {
            let mut color = self.fade_sprite.color();
            color.a = crate::ease::symmetric(4.0, fade_progress);
            self.fade_sprite.set_color(color);

            sprite_queue.draw_sprite(&self.fade_sprite);
        }

        if let Some(index) = self.active_menu {
            self.menus[index].draw(game_io, sprite_queue);
        }

        self.navigation_menu.draw(game_io, sprite_queue);
    }
}
