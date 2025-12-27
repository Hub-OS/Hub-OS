use super::{
    CategoryFilter, PackageUpdatesScene, PackagesScene, ResourceOrderScene, SyncDataScene,
};
use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::{Config, GlobalSave, InternalResolution, KeyStyle};
use crate::scenes::CreditsScene;
use framework::prelude::*;
use packets::structures::{FileHash, PackageCategory, PackageId};
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use strum::{EnumIter, IntoEnumIterator};

#[derive(Clone)]
enum Event {
    CategoryChange(ConfigCategory),
    EnterCategory,
    ApplyKeyBinds,
    OpenBindingContextMenu(flume::Sender<Option<BindingContextOption>>),
    EditVirtualControllerLayout,
    RequestNicknameChange,
    ChangeNickname {
        name: String,
    },
    SyncData,
    ClearCache,
    ViewPackages,
    UpdatePackages,
    ReceivedLatestHashes(Vec<(PackageCategory, PackageId, FileHash)>),
    ViewUpdates(Vec<(PackageCategory, PackageId, FileHash)>),
    OpenModsFolder,
    ReorderResources,
    SaveLastBattleQuestion,
    SaveLastBattle,
    ViewCredits,
    OpenLink(String),
    #[cfg(debug_assertions)]
    ViewFonts,
    Leave {
        save: bool,
    },
}

#[derive(EnumIter, Clone, Copy)]
enum ConfigCategory {
    Mods,
    Online,
    Video,
    Audio,
    Keyboard,
    Gamepad,
    About,
}

impl ConfigCategory {
    fn translation_key(self) -> &'static str {
        match self {
            ConfigCategory::Mods => "config-mods-tab",
            ConfigCategory::Online => "config-online-tab",
            ConfigCategory::Video => "config-video-tab",
            ConfigCategory::Audio => "config-audio-tab",
            ConfigCategory::Keyboard => "config-keyboard-tab",
            ConfigCategory::Gamepad => "config-gamepad-tab",
            ConfigCategory::About => "config-about-tab",
        }
    }
}

pub struct ConfigScene {
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
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
    doorstop_key: Option<TextboxDoorstopKey>,
    config: Rc<RefCell<Config>>,
    key_bindings_backup: HashMap<Input, Vec<Key>>,
    opened_virtual_controller_editor: bool,
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
            scene_title: SceneTitle::new(game_io, "config-scene-title"),
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
                .with_label(globals.translate("config-video-tab"))
                .with_children(Self::generate_mods_menu(game_io, &event_sender))
                .with_focus(false),
            cursor_sprite,
            cursor_animator,
            event_sender,
            event_receiver,
            context_menu: ContextMenu::new_translated(
                game_io,
                "config-binding-context-menu-label",
                context_position,
            )
            .with_translated_options(
                game_io,
                &[
                    ("config-binding-append-label", BindingContextOption::Append),
                    ("config-binding-clear-label", BindingContextOption::Clear),
                ],
            ),
            context_sender: None,
            textbox: Textbox::new_navigation(game_io),
            doorstop_key: None,
            config,
            key_bindings_backup,
            opened_virtual_controller_editor: false,
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
                    UiButton::new_translated(game_io, FontName::Thick, option.translation_key())
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
            ConfigCategory::Mods => Self::generate_mods_menu(game_io, event_sender),
            ConfigCategory::Online => Self::generate_online_menu(game_io, config, event_sender),
            ConfigCategory::Video => Self::generate_video_menu(game_io, config),
            ConfigCategory::Audio => Self::generate_audio_menu(game_io, config),
            ConfigCategory::Keyboard => Self::generate_keyboard_menu(game_io, config, event_sender),
            ConfigCategory::Gamepad => {
                Self::generate_controller_menu(game_io, config, event_sender)
            }
            ConfigCategory::About => Self::generate_about_menu(game_io, event_sender),
        }
    }

    fn generate_mods_menu(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        let create_button = |name: &str, event: Event| -> Box<dyn UiNode> {
            let event_sender = event_sender.clone();

            Box::new(
                UiButton::new_translated(game_io, FontName::Thick, name).on_activate(move || {
                    let _ = event_sender.send(event.clone());
                }),
            )
        };

        vec![
            create_button("config-manage-mods-label", Event::ViewPackages),
            create_button("config-update-mods-label", Event::UpdatePackages),
            create_button("config-resource-mods-label", Event::ReorderResources),
            create_button(
                "config-save-last-battle-label",
                Event::SaveLastBattleQuestion,
            ),
            #[cfg(not(target_os = "android"))]
            create_button("config-open-mods-folder-label", Event::OpenModsFolder),
        ]
    }

    fn generate_online_menu(
        game_io: &GameIO,
        config: &Rc<RefCell<Config>>,
        event_sender: &flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        let create_button = |name: &str, event: Event| -> Box<dyn UiNode> {
            let event_sender = event_sender.clone();

            Box::new(
                UiButton::new_translated(game_io, FontName::Thick, name).on_activate(move || {
                    let _ = event_sender.send(event.clone());
                }),
            )
        };

        vec![
            create_button("config-change-nickname-label", Event::RequestNicknameChange),
            create_button("config-sync-data-label", Event::SyncData),
            create_button("config-clear-cache-label", Event::ClearCache),
            Box::new(
                UiConfigNumber::new(
                    game_io,
                    "config-input-delay-label",
                    config.borrow().input_delay,
                    config.clone(),
                    |_, mut config, value| {
                        config.input_delay = value;
                    },
                )
                .with_upper_bound(MAX_INPUT_DELAY)
                .with_value_step(5),
            ),
            Box::new(UiConfigCycle::new(
                game_io,
                "config-relay-label",
                config.borrow().force_relay,
                config.clone(),
                &[("config-relay-auto", false), ("config-relay-always", true)],
                |_, mut config, value, _| {
                    config.force_relay = value;
                },
            )),
        ]
    }

    fn generate_video_menu(
        game_io: &mut GameIO,
        config: &Rc<RefCell<Config>>,
    ) -> Vec<Box<dyn UiNode>> {
        vec![
            Box::new(UiConfigToggle::new(
                game_io,
                "config-fullscreen-label",
                config.borrow().fullscreen,
                config.clone(),
                |game_io, mut config| {
                    config.fullscreen = !config.fullscreen;

                    game_io.window_mut().set_fullscreen(config.fullscreen);

                    config.fullscreen
                },
            )),
            Box::new(UiConfigToggle::new(
                game_io,
                "config-vsync-label",
                config.borrow().vsync,
                config.clone(),
                |game_io, mut config| {
                    config.vsync = !config.vsync;

                    game_io.window_mut().set_vsync_enabled(config.vsync);

                    config.vsync
                },
            )),
            Box::new(UiConfigCycle::new(
                game_io,
                "config-resolution-label",
                config.borrow().internal_resolution,
                config.clone(),
                &[
                    ("config-resolution-default", InternalResolution::Default),
                    ("config-resolution-gba", InternalResolution::Gba),
                    ("config-resolution-auto", InternalResolution::Auto),
                ],
                |game_io, mut config, value, confirmed| {
                    config.internal_resolution = value;

                    if confirmed {
                        let globals = game_io.resource_mut::<Globals>().unwrap();
                        globals.internal_resolution = value;
                    }
                },
            )),
            Box::new(UiConfigToggle::new(
                game_io,
                "config-lock-aspect-label",
                config.borrow().lock_aspect_ratio,
                config.clone(),
                |game_io, mut config| {
                    config.lock_aspect_ratio = !config.lock_aspect_ratio;

                    let window = game_io.window_mut();

                    if config.lock_aspect_ratio {
                        // lock to an initial size, more logic will occur in SupportingService
                        window.lock_resolution(RESOLUTION);
                    } else {
                        window.unlock_resolution()
                    }

                    config.lock_aspect_ratio
                },
            )),
            Box::new(UiConfigToggle::new(
                game_io,
                "config-integer-scaling-label",
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
                game_io,
                "config-snap-resize-label",
                config.borrow().snap_resize,
                config.clone(),
                |game_io, mut config| {
                    config.snap_resize = !config.snap_resize;

                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    globals.snap_resize = config.snap_resize;

                    config.snap_resize
                },
            )),
            Box::new(UiConfigNumber::new_percentage(
                game_io,
                "config-flash-brightness-label",
                config.borrow().flash_brightness,
                config.clone(),
                |_, mut config, value| {
                    config.flash_brightness = value;
                },
            )),
            Box::new(
                UiConfigNumber::new_percentage(
                    game_io,
                    "config-brightness-label",
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
            Box::new(UiConfigNumber::new_percentage(
                game_io,
                "config-saturation-label",
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
                UiConfigNumber::new_percentage(
                    game_io,
                    "config-ghosting-label",
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
                game_io,
                "config-color-sim-label",
                config.borrow().color_blindness,
                config.clone(),
                &[
                    ("config-color-sim-protanopia", 0),
                    ("config-color-sim-deuteranopia", 1),
                    ("config-color-sim-tritanopia", 2),
                    (
                        "config-color-sim-off",
                        PostProcessColorBlindness::TOTAL_OPTIONS,
                    ),
                ],
                |game_io, mut config, value, _| {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    config.color_blindness = value;
                    globals.post_process_color_blindness = value;

                    let enable = value < PostProcessColorBlindness::TOTAL_OPTIONS;
                    game_io.set_post_process_enabled::<PostProcessColorBlindness>(enable);
                },
            )),
            Box::new(UiConfigDynamicCycle::new(
                game_io,
                "config-language-label",
                config.borrow().language.clone(),
                config.clone(),
                |game_io, value| {
                    let globals = game_io.resource::<Globals>().unwrap();

                    value
                        .as_ref()
                        .map(|v| v.to_string())
                        .unwrap_or(globals.translate("config-language-auto"))
                },
                |game_io, value, right| {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let locales = globals.translations.locales();

                    let new_identifier = if let Some(value_string) = value {
                        let language_id = value_string
                            .parse::<fluent_templates::LanguageIdentifier>()
                            .ok()?;

                        let index = locales.iter().position(|id| *id == language_id)?;

                        if right {
                            locales.get(index + 1)
                        } else if index > 0 {
                            locales.get(index - 1)
                        } else {
                            None
                        }
                    } else if right {
                        locales.first()
                    } else {
                        locales.last()
                    };

                    new_identifier.map(|id| id.to_string())
                },
                |_, mut config, value| {
                    config.language = value.clone();
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
                UiConfigNumber::new_percentage(
                    game_io,
                    "config-music-label",
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
            Box::new(UiConfigNumber::new_percentage(
                game_io,
                "config-sfx-label",
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
                game_io,
                "config-mute-music-label",
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
                game_io,
                "config-mute-sfx-label",
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
                "config-audio-device-label",
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
                    let mut names: Vec<_> = std::iter::once(None)
                        .chain(AudioManager::device_names().map(Some))
                        .collect();

                    names.sort();
                    names.dedup();

                    UiConfigDynamicCycle::cycle_slice(&names, cycle_right, |name| {
                        if let Some(name) = name {
                            name == previous_value
                        } else {
                            previous_value.is_empty()
                        }
                    })
                    .cloned()
                    .unwrap_or_default()
                    .unwrap_or_default()
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
                game_io,
                "config-key-style-label",
                config.borrow().key_style,
                config.clone(),
                &[
                    ("config-key-style-mix", KeyStyle::Mix),
                    ("config-key-style-wasd", KeyStyle::Wasd),
                    ("config-key-style-emulator", KeyStyle::Emulator),
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
                UiButton::new_translated(game_io, FontName::Thick, "config-reset-binds-label")
                    .on_activate({
                        let config = config.clone();
                        let event_sender = event_sender.clone();

                        move || {
                            let mut config = config.borrow_mut();
                            config.key_bindings = Config::default_key_bindings(config.key_style);

                            event_sender.send(Event::ApplyKeyBinds).unwrap();
                        }
                    }),
            ),
        ];

        let binding_iter = Input::iter()
            .map(|option| {
                UiConfigBinding::new_keyboard(game_io, option, config.clone())
                    .with_context_requester({
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
            #[cfg(not(target_os = "android"))]
            Box::new(UiConfigDynamicCycle::new(
                game_io,
                "config-active-gamepad-label",
                config.borrow().controller_index,
                config.clone(),
                |_, value| value.to_string(),
                |game_io, previous_value, cycle_right| {
                    let controllers = game_io.input().controllers();

                    UiConfigDynamicCycle::cycle_slice(controllers, cycle_right, |controller| {
                        controller.id() == *previous_value
                    })
                    .map(|controller| controller.id())
                    .unwrap_or_default()
                },
                |_, mut config, value| {
                    config.controller_index = *value;
                },
            )),
            #[cfg(target_os = "android")]
            Box::new(
                UiButton::new_translated(
                    game_io,
                    FontName::Thick,
                    "config-edit-virtual-controller-layout-label",
                )
                .on_activate({
                    let event_sender = event_sender.clone();
                    move || {
                        let _ = event_sender.send(Event::EditVirtualControllerLayout);
                    }
                }),
            ),
            Box::new(
                UiButton::new_translated(game_io, FontName::Thick, "config-reset-binds-label")
                    .on_activate({
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
                UiConfigBinding::new_controller(game_io, option, config.clone())
                    .with_context_requester({
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

    fn generate_about_menu(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
    ) -> Vec<Box<dyn UiNode>> {
        let create_button = |name: &str, event: Event| -> Box<dyn UiNode> {
            let event_sender = event_sender.clone();

            Box::new(
                UiButton::new_translated(game_io, FontName::Thick, name).on_activate(move || {
                    let _ = event_sender.send(event.clone());
                }),
            )
        };

        vec![
            create_button("config-view-credits-label", Event::ViewCredits),
            create_button(
                "config-visit-discord-label",
                Event::OpenLink("https://discord.hubos.dev".to_string()),
            ),
            create_button(
                "config-visit-website-label",
                Event::OpenLink("https://hubos.dev".to_string()),
            ),
            #[cfg(not(target_os = "android"))]
            create_button(
                "config-view-third-party-licenses-label",
                Event::OpenLink(
                    ResourcePaths::game_folder().to_string() + "third_party_licenses.html",
                ),
            ),
            #[cfg(debug_assertions)]
            create_button("View Fonts", Event::ViewFonts),
        ]
    }
}

impl Scene for ConfigScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.textbox.use_navigation_avatar(game_io);

        let globals = game_io.resource_mut::<Globals>().unwrap();
        globals.editing_config = true;
    }

    fn exit(&mut self, game_io: &mut GameIO) {
        let globals = game_io.resource_mut::<Globals>().unwrap();
        globals.editing_config = false;
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
        self.scene_title.draw(game_io, &mut sprite_queue);

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
        let globals = game_io.resource::<Globals>().unwrap();

        if self.opened_virtual_controller_editor && !globals.editing_virtual_controller {
            self.opened_virtual_controller_editor = false;

            let message = globals.translate("config-edit-virtual-controller-layout-complete");
            self.textbox.push_interface(TextboxMessage::new(message));
            self.textbox.open();
        }

        while let Ok(event) = self.event_receiver.try_recv() {
            let globals = game_io.resource::<Globals>().unwrap();

            match event {
                Event::CategoryChange(category) => {
                    let children =
                        Self::generate_submenu(game_io, &self.config, category, &self.event_sender);

                    let globals = game_io.resource::<Globals>().unwrap();
                    let label = globals.translate(category.translation_key());
                    self.secondary_layout.set_label(label.to_uppercase());
                    self.secondary_layout.set_children(children);
                }
                Event::EnterCategory => {
                    self.primary_layout.set_focused(false);
                    self.secondary_layout.set_focused(true);
                }
                Event::ApplyKeyBinds => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    globals.config.key_bindings = self.config.borrow().key_bindings.clone();
                }
                Event::OpenBindingContextMenu(sender) => {
                    let globals = game_io.resource::<Globals>().unwrap();
                    globals.audio.play_sound(&globals.sfx.cursor_select);

                    self.context_menu.open();
                    self.context_sender = Some(sender);
                }
                Event::EditVirtualControllerLayout => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    globals.editing_virtual_controller = true;
                    self.opened_virtual_controller_editor = true;
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
                    global_save.nickname_time = GlobalSave::current_time();
                    global_save.save();
                }
                Event::SyncData => {
                    let transition = crate::transitions::new_sub_scene(game_io);
                    let scene = SyncDataScene::new(game_io);
                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }
                Event::ClearCache => {
                    let globals = &mut game_io.resource::<Globals>().unwrap();

                    let message = if !globals.connected_to_server {
                        match std::fs::remove_dir_all(ResourcePaths::server_cache_folder()) {
                            Ok(()) => globals.translate("config-clear-cache-success"),
                            Err(e) => {
                                log::error!("{e}");

                                if matches!(e.kind(), std::io::ErrorKind::NotFound) {
                                    globals.translate("config-clear-cache-already-empty")
                                } else {
                                    globals.translate("config-clear-cache-error")
                                }
                            }
                        }
                    } else {
                        globals.translate("config-clear-cache-connected-error")
                    };

                    let interface = TextboxMessage::new(message);

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                Event::ViewPackages => {
                    let scene = PackagesScene::new(game_io, CategoryFilter::default());
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }
                Event::UpdatePackages => {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let request = globals.request_latest_hashes();
                    let event_sender = self.event_sender.clone();
                    let (doorstop, doorstop_key) = TextboxDoorstop::new();
                    self.doorstop_key = Some(doorstop_key);

                    self.textbox.push_interface(doorstop.with_translated(
                        game_io,
                        "packages-checking-for-updates",
                        vec![],
                    ));
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
                    let globals = game_io.resource::<Globals>().unwrap();

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
                            TextboxMessage::new(globals.translate("packages-already-up-to-date"));
                        self.textbox.push_interface(interface);
                    } else {
                        let event_sender = self.event_sender.clone();
                        let interface =
                            TextboxMessage::new(globals.translate("packages-updates-found"))
                                .with_callback(move || {
                                    event_sender
                                        .send(Event::ViewUpdates(requires_update))
                                        .unwrap()
                                });
                        self.textbox.push_interface(interface);
                    }

                    self.doorstop_key.take();
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
                Event::SaveLastBattleQuestion => {
                    if globals.battle_recording.is_none() {
                        let interface = TextboxMessage::new(
                            globals.translate("config-save-last-battle-missing"),
                        );
                        self.textbox.push_interface(interface);
                    } else {
                        let event_sender = self.event_sender.clone();
                        let interface = TextboxQuestion::new(
                            game_io,
                            globals.translate("config-save-last-battle-question"),
                            move |save| {
                                if save {
                                    let _ = event_sender.send(Event::SaveLastBattle);
                                }
                            },
                        );
                        self.textbox.push_interface(interface);
                    }

                    self.textbox.open();
                }
                Event::SaveLastBattle => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    if let Some((props, recording, preview)) = globals.battle_recording.take() {
                        // notify the player
                        let interface =
                            TextboxMessage::new(globals.translate("config-save-last-battle-save"));
                        self.textbox.push_interface(interface);
                        self.textbox.open();

                        // save
                        recording.save(game_io, &props, preview);
                    } else {
                        log::error!("No recording? How did we get here.");
                    }
                }
                Event::OpenModsFolder => {
                    #[cfg(not(target_os = "android"))]
                    if let Err(err) =
                        opener::open(ResourcePaths::data_folder().to_string() + "mods")
                    {
                        log::error!("{err:?}")
                    }
                }
                Event::ViewCredits => {
                    let scene = CreditsScene::new(game_io);
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }
                Event::OpenLink(address) => {
                    if let Err(err) = webbrowser::open(&address) {
                        log::error!("{err:?}");
                    }
                }
                #[cfg(debug_assertions)]
                Event::ViewFonts => {
                    let scene = crate::scenes::FontTestScene::new(game_io);
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }
                Event::Leave { save } => {
                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    if save {
                        let mut new_config = self.config.borrow_mut();

                        // apply new virtual input settings, the virtual input directly modifies the global config
                        let virtual_input_positions =
                            std::mem::take(&mut globals.config.virtual_input_positions);
                        new_config.virtual_input_positions.clear();
                        new_config.virtual_controller_scale =
                            globals.config.virtual_controller_scale;

                        // save new config
                        globals.config = new_config.clone();
                        globals.config.virtual_input_positions = virtual_input_positions;
                        globals.config.save();
                    } else {
                        // reapply previous config
                        let config = &mut globals.config;
                        let mut new_config = self.config.borrow_mut();

                        // input
                        config.key_bindings = self.key_bindings_backup.clone();

                        // new config actually works as a backup for virtual input,
                        // since the virtual input editor directly modifies global config
                        config.virtual_input_positions =
                            std::mem::take(&mut new_config.virtual_input_positions);
                        config.virtual_controller_scale = new_config.virtual_controller_scale;

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
                        globals.internal_resolution = config.internal_resolution;
                        globals.snap_resize = config.snap_resize;
                        let window = game_io.window_mut();

                        window.set_fullscreen(fullscreen);

                        if lock_aspect_ratio {
                            if !window.has_locked_resolution() {
                                // lock to an initial size, more logic will occur in SupportingService
                                window.lock_resolution(RESOLUTION);
                            }
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

        if *config == globals.config && self.key_bindings_backup == config.key_bindings {
            // no changes, no need to ask if we should save
            let _ = self.event_sender.send(Event::Leave { save: false });

            globals.audio.play_sound(&globals.sfx.cursor_cancel);
            return;
        }

        let event_sender = self.event_sender.clone();

        // get permission to save
        let interface = if config.validate() {
            TextboxQuestion::new(
                game_io,
                globals.translate("config-save-changes-question"),
                move |save| {
                    let _ = event_sender.send(Event::Leave { save });
                },
            )
        } else {
            TextboxQuestion::new(
                game_io,
                globals.translate("config-invalid-revert-question"),
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
