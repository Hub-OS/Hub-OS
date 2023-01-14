use crate::bindable::SpriteColorMode;
use crate::render::ui::{
    build_9patch, Dimension, FlexDirection, FontStyle, SceneTitle, ScrollableList, Textbox,
    TextboxPrompt, TextboxQuestion, UiButton, UiConfigBinding, UiConfigToggle, UiConfigVolume,
    UiInputTracker, UiLayout, UiLayoutNode, UiNode, UiStyle,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::RefCell;
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
    next_scene: NextScene,
}

impl ConfigScene {
    pub fn new(game_io: &GameIO) -> Box<Self> {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // cursor
        let cursor_sprite = assets.new_sprite(game_io, ResourcePaths::SELECT_CURSOR);

        let mut cursor_animator =
            Animator::load_new(assets, ResourcePaths::SELECT_CURSOR_ANIMATION);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        // config
        let config = globals.config.clone();
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
        game_io: &GameIO,
        event_sender: flume::Sender<Event>,
        start: Vec2,
    ) -> UiLayout {
        let assets = &game_io.resource::<Globals>().unwrap().assets;
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
        game_io: &GameIO,
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
                config.borrow().lock_aspect_ratio,
                config.clone(),
                |game_io, mut config| {
                    config.lock_aspect_ratio = !config.lock_aspect_ratio;

                    let window = game_io.window_mut();

                    if config.lock_aspect_ratio {
                        window.lock_resolution(TRUE_RESOLUTION.into());
                    } else {
                        window.unlock_resolution()
                    }

                    config.lock_aspect_ratio
                },
            )),
        ]
    }

    fn generate_audio_menu(config: &Rc<RefCell<Config>>) -> Vec<Box<dyn UiNode>> {
        vec![
            Box::new(UiConfigVolume::new_music(config.clone())),
            Box::new(UiConfigVolume::new_sfx(config.clone())),
            Box::new(UiConfigToggle::new(
                "Mute Music",
                config.borrow().mute_music,
                config.clone(),
                |game_io, mut config| {
                    config.mute_music = !config.mute_music;

                    let audio = &mut game_io.resource_mut::<Globals>().unwrap().audio;
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

                    let audio = &mut game_io.resource_mut::<Globals>().unwrap().audio;
                    audio.set_sfx_volume(config.sfx_volume());

                    config.mute_sfx
                },
            )),
        ]
    }

    fn generate_keyboard_menu(config: &Rc<RefCell<Config>>) -> Vec<Box<dyn UiNode>> {
        Input::iter()
            .map(|option| -> Box<dyn UiNode> {
                Box::new(UiConfigBinding::new_keyboard(option, config.clone()))
            })
            .collect()
    }

    fn generate_controller_menu(config: &Rc<RefCell<Config>>) -> Vec<Box<dyn UiNode>> {
        Input::iter()
            .map(|option| -> Box<dyn UiNode> {
                Box::new(UiConfigBinding::new_controller(option, config.clone()))
            })
            .collect()
    }

    fn generate_misc_menu(
        game_io: &GameIO,
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

impl Scene for ConfigScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.update_cursor();
        self.handle_input(game_io);
        self.handle_events(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
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
    fn handle_events(&mut self, game_io: &mut GameIO) {
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
                    .with_str(&game_io.resource::<Globals>().unwrap().global_save.nickname)
                    .with_filter(|grapheme| grapheme != "\n" && grapheme != "\t")
                    .with_character_limit(8);

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                Event::ChangeNickname { name } => {
                    let global_save = &mut game_io.resource_mut::<Globals>().unwrap().global_save;
                    global_save.nickname = name;
                    global_save.save();
                }
                Event::Leave { save } => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

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
                        let lock_aspect_ratio = config.lock_aspect_ratio;
                        let window = game_io.window_mut();

                        window.set_fullscreen(fullscreen);

                        if lock_aspect_ratio {
                            window.lock_resolution(TRUE_RESOLUTION.into());
                        } else {
                            window.unlock_resolution();
                        }
                    }

                    let transition = crate::transitions::new_scene_pop(game_io);

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

    fn handle_input(&mut self, game_io: &mut GameIO) {
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

    fn handle_tab_input(&mut self, game_io: &mut GameIO) {
        self.primary_layout.update(game_io, &self.ui_input_tracker);

        let input_util = InputUtil::new(game_io);

        if !input_util.was_just_pressed(Input::Cancel) {
            // this function only handles exiting the scene
            return;
        }

        let globals = game_io.resource::<Globals>().unwrap();
        let config = self.config.borrow();

        if *config == globals.config {
            // no changes, no need to ask if we should save
            let _ = self.event_sender.send(Event::Leave { save: false });

            globals.audio.play_sound(&globals.cursor_cancel_sfx);
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

    fn handle_submenu_input(&mut self, game_io: &mut GameIO) {
        let focused_was_locked = self.secondary_layout.is_focus_locked();

        self.secondary_layout
            .update(game_io, &self.ui_input_tracker);

        if focused_was_locked || self.secondary_layout.is_focus_locked() {
            return;
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.cursor_cancel_sfx);

            self.primary_layout.set_focused(true);
            self.secondary_layout.set_focused(false);
        }
    }
}
