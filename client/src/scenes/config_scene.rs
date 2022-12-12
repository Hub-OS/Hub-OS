use crate::bindable::SpriteColorMode;
use crate::render::ui::{
    build_9patch, Dimension, FlexDirection, FontStyle, SceneTitle, ScrollableList, TextStyle,
    Textbox, TextboxPrompt, TextboxQuestion, UiButton, UiInputTracker, UiLayout, UiLayoutNode,
    UiNode, UiStyle,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use packets::structures::Direction;
use std::cell::{RefCell, RefMut};
use std::collections::HashMap;
use std::rc::Rc;
use strum::{EnumIter, IntoEnumIterator, IntoStaticStr};

enum Event {
    CategoryChange(ConfigCategory),
    EnterCategory,
    RequestNicknameChange,
    ChangeNickname { name: String },
    Leave { save: bool },
}

#[derive(EnumIter, IntoStaticStr, Clone, Copy)]
enum ConfigCategory {
    Video,
    Audio,
    Keyboard,
    Gamepad,
    Misc,
}

pub struct ConfigScene {
    camera: Camera,
    background: Background,
    ui_input_tracker: UiInputTracker,
    primary_layout: UiLayout,
    secondary_layout: ScrollableList,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    textbox: Textbox,
    config: Rc<RefCell<Config>>,
    next_scene: NextScene<Globals>,
}

impl ConfigScene {
    pub fn new(game_io: &GameIO<Globals>) -> Box<Self> {
        // cursor
        let assets = &game_io.globals().assets;
        let cursor_sprite = assets.new_sprite(game_io, ResourcePaths::SELECT_CURSOR);

        let mut cursor_animator =
            Animator::load_new(assets, ResourcePaths::SELECT_CURSOR_ANIMATION);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        // config
        let config = game_io.globals().config.clone();
        let config = Rc::new(RefCell::new(config));

        // layout positioning
        let layout_animator =
            Animator::load_new(assets, ResourcePaths::CONFIG_BG_ANIMATION).with_state("DEFAULT");

        let primary_layout_start = layout_animator.point("primary").unwrap_or_default();

        let secondary_layout_start = layout_animator.point("secondary_start").unwrap_or_default();
        let secondary_layout_end = layout_animator.point("secondary_end").unwrap_or_default();
        let secondary_bounds = Rect::from_corners(secondary_layout_start, secondary_layout_end);

        Box::new(Self {
            camera: Camera::new_ui(game_io),
            background: Background::load_static(game_io, ResourcePaths::CONFIG_BG),
            ui_input_tracker: UiInputTracker::new(),
            primary_layout: Self::generate_category_menu(
                game_io,
                event_sender.clone(),
                primary_layout_start,
            ),
            secondary_layout: ScrollableList::new(game_io, secondary_bounds, 16.0)
                .with_label_str("VIDEO")
                .with_children(Self::generate_video_menu(&config))
                .with_focus(false),
            cursor_sprite,
            cursor_animator,
            event_sender,
            event_receiver,
            textbox: Textbox::new_navigation(game_io),
            next_scene: NextScene::None,
            config,
        })
    }

    fn generate_category_menu(
        game_io: &GameIO<Globals>,
        event_sender: flume::Sender<Event>,
        start: Vec2,
    ) -> UiLayout {
        let assets = &game_io.globals().assets;
        let ui_texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let ui_animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);
        let button_9patch = build_9patch!(game_io, ui_texture.clone(), &ui_animator, "BUTTON");

        let option_style = UiStyle {
            margin_bottom: Dimension::Points(0.0),
            nine_patch: Some(button_9patch),
            ..Default::default()
        };

        UiLayout::new_vertical(
            Rect::new(start.x, start.y, f32::INFINITY, f32::INFINITY),
            ConfigCategory::iter()
                .map(|option| {
                    UiButton::new(game_io, FontStyle::Thick, option.into())
                        .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
                        .on_activate({
                            let event_sender = event_sender.clone();

                            move || {
                                let _ = event_sender.send(Event::EnterCategory);
                            }
                        })
                        .on_focus({
                            let event_sender = event_sender.clone();

                            move || {
                                let _ = event_sender.send(Event::CategoryChange(option));
                            }
                        })
                })
                .map(|node| UiLayoutNode::new(node).with_style(option_style.clone()))
                .collect(),
        )
        .with_style(UiStyle {
            flex_direction: FlexDirection::Column,
            min_width: Dimension::Undefined,
            ..Default::default()
        })
    }

    fn generate_submenu(
        game_io: &GameIO<Globals>,
        config: &Rc<RefCell<Config>>,
        category: ConfigCategory,
        event_sender: flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        match category {
            ConfigCategory::Video => Self::generate_video_menu(config),
            ConfigCategory::Audio => Self::generate_audio_menu(config),
            ConfigCategory::Keyboard => Self::generate_keyboard_menu(config),
            ConfigCategory::Gamepad => Self::generate_controller_menu(config),
            ConfigCategory::Misc => Self::generate_misc_menu(game_io, event_sender),
        }
    }

    fn generate_video_menu(config: &Rc<RefCell<Config>>) -> Vec<Box<dyn UiNode>> {
        vec![
            Box::new(UiConfigToggle::new(
                "Fullscreen",
                config.borrow().fullscreen,
                config.clone(),
                |game_io, mut config| {
                    config.fullscreen = !config.fullscreen;

                    game_io.window_mut().set_fullscreen(config.fullscreen);

                    config.fullscreen
                },
            )),
            Box::new(UiConfigToggle::new(
                "Lock Aspect",
                // config.borrow().lock_aspect_ratio,
                false,
                config.clone(),
                |_game_io, _config| {
                    // config.lock_aspect_ratio = !config.lock_aspect_ratio;

                    // game_io
                    //     .window_mut()
                    //     .set_fullscreen(config.lock_aspect_ratio);

                    // config.lock_aspect_ratio
                    false
                },
            )),
        ]
    }

    fn generate_audio_menu(config: &Rc<RefCell<Config>>) -> Vec<Box<dyn UiNode>> {
        vec![
            Box::new(UiVolume::new_music(config.clone())),
            Box::new(UiVolume::new_sfx(config.clone())),
            Box::new(UiConfigToggle::new(
                "Mute Music",
                config.borrow().mute_music,
                config.clone(),
                |game_io, mut config| {
                    config.mute_music = !config.mute_music;

                    let audio = &mut game_io.globals_mut().audio;
                    audio.set_music_volume(config.music_volume());

                    config.mute_music
                },
            )),
            Box::new(UiConfigToggle::new(
                "Mute SFX",
                config.borrow().mute_sfx,
                config.clone(),
                |game_io, mut config| {
                    config.mute_sfx = !config.mute_sfx;

                    let audio = &mut game_io.globals_mut().audio;
                    audio.set_sfx_volume(config.sfx_volume());

                    config.mute_sfx
                },
            )),
        ]
    }

    fn generate_keyboard_menu(config: &Rc<RefCell<Config>>) -> Vec<Box<dyn UiNode>> {
        Input::iter()
            .map(|option| -> Box<dyn UiNode> {
                Box::new(UiInputBinder::new_keyboard(option, config.clone()))
            })
            .collect()
    }

    fn generate_controller_menu(config: &Rc<RefCell<Config>>) -> Vec<Box<dyn UiNode>> {
        Input::iter()
            .map(|option| -> Box<dyn UiNode> {
                Box::new(UiInputBinder::new_controller(option, config.clone()))
            })
            .collect()
    }

    fn generate_misc_menu(
        game_io: &GameIO<Globals>,
        event_sender: flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        vec![Box::new(
            UiButton::new(game_io, FontStyle::Thick, "Change Nickname")
                .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
                .on_activate({
                    let event_sender = event_sender.clone();

                    move || {
                        let _ = event_sender.send(Event::RequestNicknameChange);
                    }
                }),
        )]
    }
}

impl Scene<Globals> for ConfigScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        self.update_cursor();
        self.handle_input(game_io);
        self.handle_events(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        SceneTitle::new("CONFIG").draw(game_io, &mut sprite_queue);

        self.primary_layout.draw(game_io, &mut sprite_queue);
        self.secondary_layout.draw(game_io, &mut sprite_queue);

        if !self.textbox.is_open() && self.primary_layout.focused() {
            sprite_queue.draw_sprite(&self.cursor_sprite);
        }

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

impl ConfigScene {
    fn handle_events(&mut self, game_io: &mut GameIO<Globals>) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::CategoryChange(category) => {
                    let children = Self::generate_submenu(
                        game_io,
                        &self.config,
                        category,
                        self.event_sender.clone(),
                    );

                    let label: &'static str = category.into();

                    self.secondary_layout.set_label(label.to_ascii_uppercase());
                    self.secondary_layout.set_children(children);
                }
                Event::EnterCategory => {
                    self.primary_layout.set_focused(false);
                    self.secondary_layout.set_focused(true);
                }
                Event::RequestNicknameChange => {
                    let event_sender = self.event_sender.clone();
                    let interface = TextboxPrompt::new(move |name| {
                        if !name.is_empty() {
                            let _ = event_sender.send(Event::ChangeNickname { name });
                        }
                    })
                    .with_str(&game_io.globals().global_save.nickname)
                    .with_filter(|grapheme| grapheme != "\n" && grapheme != "\t")
                    .with_character_limit(8);

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                Event::ChangeNickname { name } => {
                    let global_save = &mut game_io.globals_mut().global_save;
                    global_save.nickname = name;
                    global_save.save();
                }
                Event::Leave { save } => {
                    let globals = game_io.globals_mut();

                    if save {
                        // save new config
                        globals.config = self.config.borrow().clone();
                        globals.config.save();
                    } else {
                        // reapply previous config
                        let config = &globals.config;
                        let audio = &mut globals.audio;

                        audio.set_music_volume(config.music_volume());
                        audio.set_sfx_volume(config.sfx_volume());

                        let fullscreen = config.fullscreen;
                        game_io.window_mut().set_fullscreen(fullscreen);
                    }

                    use crate::transitions::{PushTransition, DEFAULT_PUSH_DURATION};

                    let sampler = game_io.globals().default_sampler.clone();
                    let transition = PushTransition::new(
                        game_io,
                        sampler,
                        Direction::Left,
                        DEFAULT_PUSH_DURATION,
                    );

                    self.next_scene = NextScene::new_pop().with_transition(transition);
                }
            }
        }
    }

    fn update_cursor(&mut self) {
        if !self.primary_layout.focused() {
            // skip updates if the primary menu isn't focused
            return;
        }

        let focused_index = self.primary_layout.focused_index().unwrap();
        let focused_bounds = self.primary_layout.get_bounds(focused_index).unwrap();

        self.cursor_animator.update();
        self.cursor_animator.apply(&mut self.cursor_sprite);

        let mut cursor_position = focused_bounds.center_left();
        cursor_position -= self.cursor_sprite.size() * 0.5;
        self.cursor_sprite.set_position(cursor_position);
    }

    fn handle_input(&mut self, game_io: &mut GameIO<Globals>) {
        self.ui_input_tracker.update(game_io);

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        self.textbox.update(game_io);

        if self.textbox.is_open() {
            return;
        }

        if self.primary_layout.focused() {
            self.handle_tab_input(game_io);
        } else {
            self.handle_submenu_input(game_io);
        }
    }

    fn handle_tab_input(&mut self, game_io: &mut GameIO<Globals>) {
        self.primary_layout.update(game_io, &self.ui_input_tracker);

        let input_util = InputUtil::new(game_io);

        if !input_util.was_just_pressed(Input::Cancel) {
            // this function only handles exiting the scene
            return;
        }

        let globals = game_io.globals();
        let config = self.config.borrow();

        if *config == globals.config {
            // no changes, no need to ask if we should save
            let _ = self.event_sender.send(Event::Leave { save: false });

            globals.audio.play_sound(&globals.menu_close_sfx);
            return;
        }

        let event_sender = self.event_sender.clone();

        // get permission to save
        let interface = if config.validate() {
            TextboxQuestion::new(String::from("Save changes?"), move |save| {
                let _ = event_sender.send(Event::Leave { save });
            })
        } else {
            TextboxQuestion::new(
                String::from("Config is invalid, use old config?"),
                move |leave| {
                    if leave {
                        let _ = event_sender.send(Event::Leave { save: false });
                    }
                },
            )
        };

        self.textbox.push_interface(interface);
        self.textbox.open();
    }

    fn handle_submenu_input(&mut self, game_io: &mut GameIO<Globals>) {
        let focused_was_locked = self.secondary_layout.is_focus_locked();

        self.secondary_layout
            .update(game_io, &self.ui_input_tracker);

        if focused_was_locked || self.secondary_layout.is_focus_locked() {
            return;
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.cursor_cancel_sfx);

            self.primary_layout.set_focused(true);
            self.secondary_layout.set_focused(false);
        }
    }
}

struct UiConfigToggle {
    name: &'static str,
    value: bool,
    config: Rc<RefCell<Config>>,
    callback: Box<dyn Fn(&mut GameIO<Globals>, RefMut<Config>) -> bool>,
}

impl UiConfigToggle {
    fn new(
        name: &'static str,
        value: bool,
        config: Rc<RefCell<Config>>,
        callback: impl Fn(&mut GameIO<Globals>, RefMut<Config>) -> bool + 'static,
    ) -> Self {
        Self {
            name,
            config,
            value,
            callback: Box::new(callback),
        }
    }
}

impl UiNode for UiConfigToggle {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO<Globals>,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        // draw name
        text_style.draw(game_io, sprite_queue, self.name);

        // draw value
        let text = if self.value { "true" } else { "false" };
        let metrics = text_style.measure(text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, text);
    }

    fn measure_ui_size(&mut self, _: &GameIO<Globals>) -> Vec2 {
        Vec2::ZERO
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>, _bounds: Rect, focused: bool) {
        if !focused {
            return;
        }

        let input_util = InputUtil::new(game_io);

        let confirmed = input_util.was_just_pressed(Input::Confirm)
            || input_util.was_just_pressed(Input::Left)
            || input_util.was_just_pressed(Input::Right);

        if !confirmed {
            return;
        }

        let original_value = self.value;
        self.value = (self.callback)(game_io, self.config.borrow_mut());

        let globals = game_io.globals();

        if self.value == original_value {
            // toggle failed
            globals.audio.play_sound(&globals.cursor_error_sfx);
        } else {
            globals.audio.play_sound(&globals.cursor_select_sfx);
        }
    }
}

struct UiVolume {
    sfx: bool,
    level_text: String,
    locking_focus: bool,
    ui_input_tracker: UiInputTracker,
    config: Rc<RefCell<Config>>,
}

impl UiVolume {
    fn new(sfx: bool, config: Rc<RefCell<Config>>) -> Self {
        let level = if sfx {
            config.borrow().sfx
        } else {
            config.borrow().music
        };

        Self {
            sfx,
            level_text: Self::generate_text(level),
            locking_focus: false,
            ui_input_tracker: UiInputTracker::new(),
            config,
        }
    }

    fn new_music(config: Rc<RefCell<Config>>) -> Self {
        Self::new(false, config)
    }

    fn new_sfx(config: Rc<RefCell<Config>>) -> Self {
        Self::new(true, config)
    }

    fn generate_text(level: u8) -> String {
        format!("{}/{}", level, MAX_VOLUME)
    }
}

impl UiNode for UiVolume {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO<Globals>,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        // draw name
        let name = if self.sfx { "SFX" } else { "Music" };
        text_style.draw(game_io, sprite_queue, name);

        // draw value
        text_style.monospace = true;
        let metrics = text_style.measure(&self.level_text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, &self.level_text);
    }

    fn measure_ui_size(&mut self, _: &GameIO<Globals>) -> Vec2 {
        Vec2::ZERO
    }

    fn is_locking_focus(&self) -> bool {
        self.locking_focus
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>, _bounds: Rect, focused: bool) {
        self.ui_input_tracker.update(game_io);

        if !focused {
            return;
        }

        let input_util = InputUtil::new(game_io);

        let confirm = input_util.was_just_pressed(Input::Confirm);
        let left = self.ui_input_tracker.is_active(Input::Left);
        let right = self.ui_input_tracker.is_active(Input::Right);

        if !self.locking_focus {
            if left || right || confirm {
                self.locking_focus = true;
            } else {
                return;
            }
        } else if confirm || input_util.was_just_pressed(Input::Cancel) {
            self.locking_focus = false;
            return;
        }

        if !self.locking_focus {
            // can't change volume unless the input is selected
            return;
        }

        let mut config = self.config.borrow_mut();
        let level = if self.sfx {
            &mut config.sfx
        } else {
            &mut config.music
        };

        let original_level = *level;

        if left && *level > 0 {
            *level -= 1;
        }

        if right && *level < MAX_VOLUME {
            *level += 1;
        }

        if *level == original_level {
            // no change, no need to update anything
            return;
        }

        // update visual
        self.level_text = Self::generate_text(*level);

        // update and test volume
        let volume = if self.sfx {
            config.sfx_volume()
        } else {
            config.music_volume()
        };

        let globals = game_io.globals_mut();
        let audio = &mut globals.audio;

        if self.sfx {
            audio.set_sfx_volume(volume);
            audio.play_sound(&globals.cursor_move_sfx);
        } else {
            audio.set_music_volume(volume);
        }
    }
}

struct UiInputBinder {
    binds_keyboard: bool,
    input: Input,
    config: Rc<RefCell<Config>>,
    binding: bool,
}

impl UiInputBinder {
    fn new_keyboard(input: Input, config: Rc<RefCell<Config>>) -> Self {
        Self {
            binds_keyboard: true,
            input,
            config,
            binding: false,
        }
    }

    fn new_controller(input: Input, config: Rc<RefCell<Config>>) -> Self {
        Self {
            binds_keyboard: false,
            input,
            config,
            binding: false,
        }
    }

    fn bound_text(&self) -> Option<&'static str> {
        let config = self.config.borrow();

        if self.binds_keyboard {
            let key = *config.key_bindings.get(&self.input)?;

            Some(key.into())
        } else {
            let button = *config.controller_bindings.get(&self.input)?;

            let text = match button {
                Button::LeftShoulder => "L-Shldr",
                Button::LeftTrigger => "L-Trgger",
                Button::RightShoulder => "R-Shldr",
                Button::RightTrigger => "R-Trgger",
                Button::LeftStick => "L-Stick",
                Button::RightStick => "R-Stick",
                Button::LeftStickUp => "L-Up",
                Button::LeftStickDown => "L-Down",
                Button::LeftStickLeft => "L-Left",
                Button::LeftStickRight => "L-Right",
                Button::RightStickUp => "R-Up",
                Button::RightStickDown => "R-Down",
                Button::RightStickLeft => "R-Left",
                Button::RightStickRight => "R-Right",
                button => button.into(),
            };

            Some(text)
        }
    }
}

impl UiNode for UiInputBinder {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO<Globals>,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        let bound_text = self.bound_text();

        if Input::REQUIRED_INPUTS.contains(&self.input) && bound_text.is_none() {
            // display unbound required inputs in red
            text_style.color = Color::ORANGE;
        }

        // draw input name
        let text: &'static str = match self.input {
            Input::AdvanceFrame => "AdvFrame",
            Input::RewindFrame => "RewFrame",
            input => input.into(),
        };

        text_style.draw(game_io, sprite_queue, text);

        // draw binding
        let text: &'static str = if self.binding {
            "..."
        } else {
            bound_text.unwrap_or_default()
        };

        let metrics = text_style.measure(text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, text);
    }

    fn measure_ui_size(&mut self, _: &GameIO<Globals>) -> Vec2 {
        Vec2::ZERO
    }

    fn focusable(&self) -> bool {
        true
    }

    fn is_locking_focus(&self) -> bool {
        self.binding
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>, _bounds: Rect, focused: bool) {
        if !focused {
            return;
        }

        if !self.binding {
            let input_util = InputUtil::new(game_io);

            // erase binding
            if input_util.was_just_pressed(Input::Option) {
                let globals = game_io.globals();
                globals.audio.play_sound(&globals.cursor_cancel_sfx);

                let mut config = self.config.borrow_mut();

                if self.binds_keyboard {
                    config.key_bindings.remove(&self.input);
                } else {
                    config.controller_bindings.remove(&self.input);
                }
            }

            // begin new binding
            if input_util.was_just_pressed(Input::Confirm) {
                let globals = game_io.globals();
                globals.audio.play_sound(&globals.cursor_select_sfx);

                self.binding = true;
            }
            return;
        }

        if self.binds_keyboard {
            if let Some(key) = game_io.input().latest_key() {
                let mut config = self.config.borrow_mut();
                Self::bind(&mut config.key_bindings, self.input, key);
                self.binding = false;
            }
        } else {
            if game_io.input().latest_key().is_some() {
                self.binding = false;
            }

            if let Some(button) = game_io.input().latest_button() {
                let mut config = self.config.borrow_mut();
                Self::bind(&mut config.controller_bindings, self.input, button);
                self.binding = false;
            }
        }
    }
}

impl UiInputBinder {
    fn bind<V: std::cmp::PartialEq + Clone>(
        bindings: &mut HashMap<Input, V>,
        input: Input,
        value: V,
    ) {
        if Input::REQUIRED_INPUTS.contains(&input) {
            // unbind overlapping input
            if let Some(input) = bindings.iter().find(|(_, v)| **v == value).map(|(k, _)| *k) {
                bindings.remove(&input);
            }
        }

        bindings.insert(input, value);
    }
}
