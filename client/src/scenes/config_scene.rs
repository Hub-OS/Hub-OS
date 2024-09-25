use super::{CategoryFilter, PackageUpdatesScene, PackagesScene, ResourceOrderScene};
use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::{Config, KeyStyle};
use framework::prelude::*;
use packets::structures::{FileHash, PackageCategory, PackageId};
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use strum::{EnumIter, IntoEnumIterator, IntoStaticStr};

#[derive(Clone)]
enum Event {
    CategoryChange(ConfigCategory),
    EnterCategory,
    OpenBindingContextMenu(flume::Sender<Option<BindingContextOption>>),
    RequestNicknameChange,
    ChangeNickname { name: String },
    ViewPackages,
    UpdatePackages,
    ReceivedLatestHashes(Vec<(PackageCategory, PackageId, FileHash)>),
    ViewUpdates(Vec<(PackageCategory, PackageId, FileHash)>),
    ReorderResources,
    ClearCache,
    Leave { save: bool },
}

#[derive(EnumIter, IntoStaticStr, Clone, Copy)]
enum ConfigCategory {
    Video,
    Audio,
    Keyboard,
    Gamepad,
    Mods,
    Profile,
}

pub struct ConfigScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    ui_input_tracker: UiInputTracker,
    primary_layout: UiLayout,
    secondary_layout: ScrollableList,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    context_menu: ContextMenu<BindingContextOption>,
    context_sender: Option<flume::Sender<Option<BindingContextOption>>>,
    textbox: Textbox,
    doorstop_remover: Option<TextboxDoorstopRemover>,
    config: Rc<RefCell<Config>>,
    key_bindings_backup: HashMap<Input, Vec<Key>>,
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
        let key_bindings_backup = config.key_bindings.clone();
        let config = Rc::new(RefCell::new(config));

        // layout positioning
        let ui_animator =
            Animator::load_new(assets, ResourcePaths::CONFIG_UI_ANIMATION).with_state("DEFAULT");

        let primary_layout_start = ui_animator.point_or_zero("PRIMARY");

        let secondary_layout_start = ui_animator.point_or_zero("SECONDARY_START");
        let secondary_layout_end = ui_animator.point_or_zero("SECONDARY_END");
        let secondary_bounds = Rect::from_corners(secondary_layout_start, secondary_layout_end);

        let context_position = ui_animator.point_or_zero("CONTEXT_MENU");

        Box::new(Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_arms(true),
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
            context_menu: ContextMenu::new(game_io, "BINDING", context_position).with_options(
                game_io,
                [
                    ("Append", BindingContextOption::Append),
                    ("Clear", BindingContextOption::Clear),
                ],
            ),
            context_sender: None,
            textbox: Textbox::new_navigation(game_io),
            doorstop_remover: None,
            config,
            key_bindings_backup,
            next_scene: NextScene::None,
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
        let button_9patch = build_9patch!(game_io, ui_texture, &ui_animator, "BUTTON");

        let option_style = UiStyle {
            margin_bottom: LengthPercentageAuto::Points(0.0),
            nine_patch: Some(button_9patch),
            ..Default::default()
        };

        UiLayout::new_vertical(
            Rect::new(start.x, start.y, f32::INFINITY, f32::INFINITY),
            ConfigCategory::iter()
                .map(|option| {
                    UiButton::new_text(game_io, FontName::Thick, option.into())
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
            min_width: Dimension::Auto,
            ..Default::default()
        })
    }

    fn generate_submenu(
        game_io: &mut GameIO,
        config: &Rc<RefCell<Config>>,
        category: ConfigCategory,
        event_sender: &flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        match category {
            ConfigCategory::Video => Self::generate_video_menu(config),
            ConfigCategory::Audio => Self::generate_audio_menu(game_io, config),
            ConfigCategory::Keyboard => Self::generate_keyboard_menu(game_io, config, event_sender),
            ConfigCategory::Gamepad => {
                Self::generate_controller_menu(game_io, config, event_sender)
            }
            ConfigCategory::Mods => Self::generate_mods_menu(game_io, event_sender),
            ConfigCategory::Profile => Self::generate_profile_menu(game_io, event_sender),
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
                "VSync",
                config.borrow().vsync,
                config.clone(),
                |game_io, mut config| {
                    config.vsync = !config.vsync;

                    game_io.window_mut().set_vsync_enabled(config.vsync);

                    config.vsync
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
                        window.lock_resolution(TRUE_RESOLUTION);
                    } else {
                        window.unlock_resolution()
                    }

                    config.lock_aspect_ratio
                },
            )),
            Box::new(UiConfigToggle::new(
                "Integer Scaling",
                config.borrow().integer_scaling,
                config.clone(),
                |game_io, mut config| {
                    config.integer_scaling = !config.integer_scaling;

                    let window = game_io.window_mut();
                    window.set_integer_scaling(config.integer_scaling);

                    config.integer_scaling
                },
            )),
            Box::new(UiConfigToggle::new(
                "Snap Resize",
                config.borrow().snap_resize,
                config.clone(),
                |game_io, mut config| {
                    config.snap_resize = !config.snap_resize;

                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    globals.snap_resize = config.snap_resize;

                    config.snap_resize
                },
            )),
            Box::new(
                UiConfigPercentage::new(
                    "Brightness",
                    config.borrow().brightness,
                    config.clone(),
                    |game_io, mut config, value| {
                        let globals = game_io.resource_mut::<Globals>().unwrap();

                        config.brightness = value;
                        globals.post_process_adjust_config =
                            PostProcessAdjustConfig::from_config(&config);

                        let enable = globals.post_process_adjust_config.should_enable();
                        game_io.set_post_process_enabled::<PostProcessAdjust>(enable);
                    },
                )
                .with_lower_bound(10),
            ),
            Box::new(UiConfigPercentage::new(
                "Saturation",
                config.borrow().saturation,
                config.clone(),
                |game_io, mut config, value| {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    config.saturation = value;
                    globals.post_process_adjust_config =
                        PostProcessAdjustConfig::from_config(&config);

                    let enable = globals.post_process_adjust_config.should_enable();
                    game_io.set_post_process_enabled::<PostProcessAdjust>(enable);
                },
            )),
            Box::new(
                UiConfigPercentage::new(
                    "Ghosting",
                    config.borrow().ghosting,
                    config.clone(),
                    |game_io, mut config, value| {
                        let globals = game_io.resource_mut::<Globals>().unwrap();

                        config.ghosting = value;
                        globals.post_process_ghosting = value as f32 * 0.01;

                        let enable = value > 0;
                        game_io.set_post_process_enabled::<PostProcessGhosting>(enable);
                    },
                )
                .with_upper_bound(98),
            ),
            Box::new(UiConfigCycle::new(
                "Color Sim",
                config.borrow().color_blindness,
                config.clone(),
                &[
                    ("Prot", 0),
                    ("Deut", 1),
                    ("Trit", 2),
                    ("Off", PostProcessColorBlindness::TOTAL_OPTIONS),
                ],
                |game_io, mut config, value, _| {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    config.color_blindness = value;
                    globals.post_process_color_blindness = value;

                    let enable = value < PostProcessColorBlindness::TOTAL_OPTIONS;
                    game_io.set_post_process_enabled::<PostProcessColorBlindness>(enable);
                },
            )),
        ]
    }

    fn generate_audio_menu(
        game_io: &mut GameIO,
        config: &Rc<RefCell<Config>>,
    ) -> Vec<Box<dyn UiNode>> {
        vec![
            Box::new(
                UiConfigPercentage::new(
                    "Music",
                    config.borrow().music,
                    config.clone(),
                    |game_io, mut config, value| {
                        let globals = game_io.resource_mut::<Globals>().unwrap();
                        let audio = &mut globals.audio;

                        config.music = value;
                        if !config.mute_music {
                            audio.set_music_volume(value as f32 * 0.01);
                        }
                    },
                )
                .with_auditory_feedback(false),
            ),
            Box::new(UiConfigPercentage::new(
                "SFX",
                config.borrow().sfx,
                config.clone(),
                |game_io, mut config, value| {
                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    let audio = &mut globals.audio;

                    config.sfx = value;

                    if !config.mute_sfx {
                        audio.set_sfx_volume(value as f32 * 0.01);
                    }
                },
            )),
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
            Box::new(UiConfigDynamicCycle::new(
                game_io,
                "Device",
                config.borrow().audio_device.clone(),
                config.clone(),
                |_, value| {
                    if value.is_empty() {
                        String::from("Auto")
                    } else {
                        value.clone()
                    }
                },
                |_, previous_value, cycle_right| {
                    let names: Vec<_> = std::iter::once(None)
                        .chain(AudioManager::device_names().map(Some))
                        .collect();

                    let device_name =
                        UiConfigDynamicCycle::cycle_slice(&names, cycle_right, |name| {
                            if let Some(name) = name {
                                name == previous_value
                            } else {
                                previous_value.is_empty()
                            }
                        })
                        .cloned()
                        .unwrap_or_default()
                        .unwrap_or_default();

                    device_name
                },
                |game_io, mut config, value| {
                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    globals.audio.use_device(value);

                    config.audio_device.clone_from(value);
                },
            )),
        ]
    }

    fn generate_keyboard_menu(
        game_io: &GameIO,
        config: &Rc<RefCell<Config>>,
        event_sender: &flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        let mut children: Vec<Box<dyn UiNode>> = vec![
            Box::new(UiConfigCycle::new(
                "Style",
                config.borrow().key_style,
                config.clone(),
                &[
                    ("Mix", KeyStyle::Mix),
                    ("WASD", KeyStyle::Wasd),
                    ("Emulator", KeyStyle::Emulator),
                ],
                |game_io, mut config, value, confirmed| {
                    config.key_style = value;
                    config.key_bindings = Config::default_key_bindings(config.key_style);

                    if confirmed {
                        let globals = game_io.resource_mut::<Globals>().unwrap();
                        globals.config.key_bindings = config.key_bindings.clone();
                    }
                },
            )),
            Box::new(
                UiButton::new_text(game_io, FontName::Thick, "Reset Binds").on_activate({
                    let config = config.clone();

                    move || {
                        let mut config = config.borrow_mut();
                        config.key_bindings = Config::default_key_bindings(config.key_style);
                    }
                }),
            ),
        ];

        let binding_iter = Input::iter()
            .map(|option| {
                UiConfigBinding::new_keyboard(option, config.clone()).with_context_requester({
                    let event_sender = event_sender.clone();
                    move |sender| {
                        event_sender
                            .send(Event::OpenBindingContextMenu(sender))
                            .unwrap();
                    }
                })
            })
            .map(|ui_node| -> Box<dyn UiNode> { Box::new(ui_node) });

        children.extend(binding_iter);

        children
    }

    fn generate_controller_menu(
        game_io: &mut GameIO,
        config: &Rc<RefCell<Config>>,
        event_sender: &flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        let mut children: Vec<Box<dyn UiNode>> = vec![
            Box::new(UiConfigDynamicCycle::new(
                game_io,
                "Active Gamepad",
                config.borrow().controller_index,
                config.clone(),
                |_, value| value.to_string(),
                |game_io, previous_value, cycle_right| {
                    let controllers = game_io.input().controllers();

                    let id =
                        UiConfigDynamicCycle::cycle_slice(controllers, cycle_right, |controller| {
                            controller.id() == *previous_value
                        })
                        .map(|controller| controller.id())
                        .unwrap_or_default();

                    id
                },
                |_, mut config, value| {
                    config.controller_index = *value;
                },
            )),
            Box::new(
                UiButton::new_text(game_io, FontName::Thick, "Reset Binds").on_activate({
                    let config = config.clone();

                    move || {
                        let mut config = config.borrow_mut();
                        config.controller_bindings = Config::default_controller_bindings();
                    }
                }),
            ),
        ];

        let binding_iter = Input::iter()
            .map(|option| {
                UiConfigBinding::new_controller(option, config.clone()).with_context_requester({
                    let event_sender = event_sender.clone();
                    move |sender| {
                        event_sender
                            .send(Event::OpenBindingContextMenu(sender))
                            .unwrap();
                    }
                })
            })
            .map(|ui_node| -> Box<dyn UiNode> { Box::new(ui_node) });

        children.extend(binding_iter);

        children
    }

    fn generate_mods_menu(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        let create_button = |name: &str, event: Event| -> Box<dyn UiNode> {
            let event_sender = event_sender.clone();

            Box::new(
                UiButton::new_text(game_io, FontName::Thick, name).on_activate(move || {
                    let _ = event_sender.send(event.clone());
                }),
            )
        };

        vec![
            create_button("Manage Mods", Event::ViewPackages),
            create_button("Update Mods", Event::UpdatePackages),
            create_button("Resource Mods", Event::ReorderResources),
            create_button("Clear Cache", Event::ClearCache),
        ]
    }

    fn generate_profile_menu(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        let create_button = |name: &str, event: Event| -> Box<dyn UiNode> {
            let event_sender = event_sender.clone();

            Box::new(
                UiButton::new_text(game_io, FontName::Thick, name).on_activate(move || {
                    let _ = event_sender.send(event.clone());
                }),
            )
        };

        vec![create_button(
            "Change Nickname",
            Event::RequestNicknameChange,
        )]
    }
}

impl Scene for ConfigScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.textbox.use_navigation_avatar(game_io);
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.update_cursor();
        self.handle_input(game_io);
        self.handle_events(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        self.frame.draw(&mut sprite_queue);
        SceneTitle::new("CONFIG").draw(game_io, &mut sprite_queue);

        self.primary_layout.draw(game_io, &mut sprite_queue);
        self.secondary_layout.draw(game_io, &mut sprite_queue);

        if !self.textbox.is_open() && self.primary_layout.focused() {
            sprite_queue.draw_sprite(&self.cursor_sprite);
        }

        self.context_menu.draw(game_io, &mut sprite_queue);

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

impl ConfigScene {
    fn handle_events(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::CategoryChange(category) => {
                    let children =
                        Self::generate_submenu(game_io, &self.config, category, &self.event_sender);

                    let label: &'static str = category.into();

                    self.secondary_layout.set_label(label.to_ascii_uppercase());
                    self.secondary_layout.set_children(children);
                }
                Event::OpenBindingContextMenu(sender) => {
                    let globals = game_io.resource::<Globals>().unwrap();
                    globals.audio.play_sound(&globals.sfx.cursor_select);

                    self.context_menu.open();
                    self.context_sender = Some(sender);
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
                Event::ViewPackages => {
                    let scene = PackagesScene::new(game_io, CategoryFilter::default());
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }
                Event::UpdatePackages => {
                    let globals = &mut game_io.resource::<Globals>().unwrap();
                    let request = globals.request_latest_hashes();
                    let event_sender = self.event_sender.clone();
                    let (doorstop, doorstop_remover) = TextboxDoorstop::new();
                    self.doorstop_remover = Some(doorstop_remover);

                    self.textbox
                        .push_interface(doorstop.with_str("Checking for updates..."));
                    self.textbox.open();

                    game_io
                        .spawn_local_task(async move {
                            let results = request.await;
                            let event = Event::ReceivedLatestHashes(results);
                            event_sender.send(event).unwrap();
                        })
                        .detach();
                }
                Event::ReceivedLatestHashes(results) => {
                    let globals = &mut game_io.resource::<Globals>().unwrap();

                    let requires_update: Vec<_> = results
                        .into_iter()
                        .filter(|(category, id, hash)| {
                            let Some(package_info) =
                                globals.package_info(*category, PackageNamespace::Local, id)
                            else {
                                return false;
                            };

                            package_info.hash != *hash
                        })
                        .collect();

                    if requires_update.is_empty() {
                        let interface =
                            TextboxMessage::new(String::from("All packages are up to date."));
                        self.textbox.push_interface(interface);
                    } else {
                        let event_sender = self.event_sender.clone();
                        let interface = TextboxMessage::new(String::from("Updates found."))
                            .with_callback(move || {
                                event_sender
                                    .send(Event::ViewUpdates(requires_update))
                                    .unwrap()
                            });
                        self.textbox.push_interface(interface);
                    }

                    let remove_doorstop = self.doorstop_remover.take().unwrap();
                    remove_doorstop();
                }
                Event::ViewUpdates(requires_update) => {
                    let transition = crate::transitions::new_sub_scene(game_io);
                    let scene = PackageUpdatesScene::new(game_io, requires_update);
                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }
                Event::ReorderResources => {
                    let transition = crate::transitions::new_sub_scene(game_io);
                    let scene = ResourceOrderScene::new(game_io);
                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }
                Event::ClearCache => {
                    let globals = &mut game_io.resource::<Globals>().unwrap();

                    let message = if !globals.connected_to_server {
                        match std::fs::remove_dir_all(ResourcePaths::SERVER_CACHE_FOLDER) {
                            Ok(()) => String::from("Successfully cleared cache."),
                            Err(e) => {
                                log::error!("{e}");

                                if matches!(e.kind(), std::io::ErrorKind::NotFound) {
                                    String::from("Cache is already empty.")
                                } else {
                                    String::from("Failed to clear cache.")
                                }
                            }
                        }
                    } else {
                        String::from("You should jack out before clearing cache.")
                    };

                    let interface = TextboxMessage::new(message);

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                Event::Leave { save } => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    if save {
                        // save new config
                        globals.config = self.config.borrow().clone();
                        globals.config.save();
                    } else {
                        // reapply previous config
                        let config = &mut globals.config;

                        // input
                        config.key_bindings = self.key_bindings_backup.clone();

                        // audio
                        let audio = &mut globals.audio;

                        audio.set_music_volume(config.music_volume());
                        audio.set_sfx_volume(config.sfx_volume());

                        // post processing
                        globals.post_process_adjust_config =
                            PostProcessAdjustConfig::from_config(config);
                        globals.post_process_ghosting = config.ghosting as f32 * 0.01;

                        let enable_adjustment = globals.post_process_adjust_config.should_enable();
                        let enable_ghosting = config.ghosting > 0;
                        let enable_color_blindness =
                            config.color_blindness < PostProcessColorBlindness::TOTAL_OPTIONS;

                        // window
                        let fullscreen = config.fullscreen;
                        let lock_aspect_ratio = config.lock_aspect_ratio;
                        let integer_scaling = config.integer_scaling;
                        globals.snap_resize = config.snap_resize;
                        let window = game_io.window_mut();

                        window.set_fullscreen(fullscreen);

                        if lock_aspect_ratio {
                            window.lock_resolution(TRUE_RESOLUTION);
                        } else {
                            window.unlock_resolution();
                        }

                        window.set_integer_scaling(integer_scaling);

                        // post processing again
                        game_io.set_post_process_enabled::<PostProcessAdjust>(enable_adjustment);
                        game_io.set_post_process_enabled::<PostProcessGhosting>(enable_ghosting);
                        game_io.set_post_process_enabled::<PostProcessColorBlindness>(
                            enable_color_blindness,
                        );
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

        if game_io.is_in_transition() {
            return;
        }

        self.handle_context_menu_input(game_io);

        if self.primary_layout.focused() {
            self.handle_tab_input(game_io);
        } else {
            self.handle_submenu_input(game_io);
        }
    }

    fn handle_context_menu_input(&mut self, game_io: &mut GameIO) {
        if !self.context_menu.is_open() {
            if let Some(sender) = self.context_sender.take() {
                let _ = sender.send(None);
            }
            return;
        }

        if let Some(selection) = self.context_menu.update(game_io, &self.ui_input_tracker) {
            if let Some(sender) = self.context_sender.take() {
                let _ = sender.send(Some(selection));
            }
            self.context_menu.close();
        }
    }

    fn handle_tab_input(&mut self, game_io: &mut GameIO) {
        self.primary_layout.update(game_io, &self.ui_input_tracker);

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Right) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.primary_layout.set_focused(false);
            self.secondary_layout.set_focused(true);
            return;
        }

        if !input_util.was_just_pressed(Input::Cancel) {
            // the rest of this function only handles exiting the scene
            return;
        }

        let globals = game_io.resource::<Globals>().unwrap();
        let config = self.config.borrow();

        if *config == globals.config {
            // no changes, no need to ask if we should save
            let _ = self.event_sender.send(Event::Leave { save: false });

            globals.audio.play_sound(&globals.sfx.cursor_cancel);
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

        if input_util.was_just_pressed(Input::Left) || input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            self.primary_layout.set_focused(true);
            self.secondary_layout.set_focused(false);
        }
    }
}
