use crate::bindable::SpriteColorMode;
use crate::packages::{Package, PackageNamespace, ResourcePackage};
use crate::render::ui::{
    ContextMenu, FontName, InputTip, PackageListing, SceneTitle, ScrollTracker, ScrollableFrame,
    SubSceneFrame, TextStyle, Textbox, TextboxMessage, TextboxQuestion, UiInputTracker,
};
use crate::render::{Animator, Background, Camera, SpriteColorQueue};
use crate::resources::{DIM_TEXT_COLOR, Globals, ResourcePaths, TEXT_DARK_SHADOW_COLOR};
use crate::saves::GlobalSave;
use framework::prelude::*;
use packets::structures::{Input, PackageId};

use super::PackageScene;

#[derive(Clone, Copy)]
enum MenuOption {
    View,
    Move,
}

enum Event {
    SaveResponse { save: bool },
    Exit,
}

pub struct ResourceOrderScene {
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
    frame: SubSceneFrame,
    input_tip: InputTip,
    enabled_string: String,
    disabled_string: String,
    list_bounds: Rect,
    scrollable_frame: ScrollableFrame,
    scroll_tracker: ScrollTracker,
    ui_input_tracker: UiInputTracker,
    package_order: Vec<(PackageListing, bool)>,
    moving_package: bool,
    context_menu: ContextMenu<MenuOption>,
    textbox: Textbox,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    next_scene: NextScene,
}

impl ResourceOrderScene {
    pub fn new(game_io: &mut GameIO) -> Self {
        let globals = Globals::from_resources_mut(game_io);
        let assets = &globals.assets;

        // load layout
        let ui_animator = Animator::load_new(assets, ResourcePaths::RESOURCE_ORDER_UI_ANIMATION)
            .with_state("DEFAULT");

        let option_tip_top_right = ui_animator.point_or_zero("MENU_TIP_TOP_RIGHT");

        let frame_bounds = Rect::from_corners(
            ui_animator.point_or_zero("LIST_START"),
            ui_animator.point_or_zero("LIST_END"),
        );

        let list_padding = ui_animator.point_or_zero("LIST_PADDING");
        let context_position = ui_animator.point_or_zero("CONTEXT_MENU");

        // define frame region
        let scrollable_frame = ScrollableFrame::new(game_io, frame_bounds);
        let cursor_start = scrollable_frame.body_bounds().top_left() + Vec2::new(-8.0, 2.0);

        let mut scroll_tracker = ScrollTracker::new(game_io, 7);
        scroll_tracker.define_cursor(cursor_start, 16.0);
        scroll_tracker.define_scrollbar(
            scrollable_frame.scroll_start(),
            scrollable_frame.scroll_end(),
        );

        // define list region
        let mut list_bounds = scrollable_frame.body_bounds();
        list_bounds.x += list_padding.x;
        list_bounds.width -= list_padding.x * 2.0;
        list_bounds.y += list_padding.y;
        list_bounds.height -= list_padding.y * 2.0;

        // resolve packages
        let globals = Globals::from_resources_mut(game_io);
        let packages = &globals.resource_packages;

        let global_save = &mut globals.global_save;

        if global_save.include_new_resources(&globals.resource_packages) {
            global_save.save();
        }

        let package_order_iter =
            global_save
                .resource_package_order
                .iter()
                .flat_map(|(id, enabled)| {
                    let package = packages.package_or_fallback(PackageNamespace::Local, id)?;

                    Some((package.create_package_listing(), *enabled))
                });

        let mut package_order = vec![(ResourcePackage::default_package_listing(), true)];
        package_order.extend(package_order_iter);

        scroll_tracker.set_total_items(package_order.len());

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        // strings
        let enabled_string = globals.translate("config-resource-order-enabled-label");
        let disabled_string = globals.translate("config-resource-order-disabled-label");

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            scene_title: SceneTitle::new(game_io, "config-resource-order-scene-title"),
            frame: SubSceneFrame::new(game_io).with_everything(true),
            input_tip: InputTip::new(
                game_io,
                Input::Option,
                "config-resource-order-context-menu-tip",
                option_tip_top_right,
            ),
            enabled_string,
            disabled_string,
            list_bounds,
            scrollable_frame,
            scroll_tracker,
            ui_input_tracker: UiInputTracker::new(),
            package_order,
            moving_package: false,
            context_menu: ContextMenu::new_translated(
                game_io,
                "config-resource-order-context-menu-label",
                context_position,
            ),
            textbox: Textbox::new_navigation(game_io),
            event_sender,
            event_receiver,
            next_scene: NextScene::None,
        }
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        self.ui_input_tracker.update(game_io);

        let use_idle_cursor = self.moving_package || self.context_menu.is_open();
        self.scroll_tracker.set_idle(use_idle_cursor);

        if self.textbox.is_open() {
            return;
        }

        if self.context_menu.is_open() {
            self.update_context_menu(game_io);
            return;
        }

        let globals = Globals::from_resources(game_io);

        // handle cancel request
        if self.ui_input_tracker.pulsed(Input::Cancel) {
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            if self.moving_package {
                self.moving_package = false;
            } else if self.collect_order() == globals.global_save.resource_package_order {
                // no changes made, no need to save
                self.exit(game_io);
            } else {
                // changes made, must verify
                let event_sender = self.event_sender.clone();
                let question = globals.translate("config-resource-order-save-changes-question");
                let interface = TextboxQuestion::new(game_io, question, move |save| {
                    event_sender.send(Event::SaveResponse { save }).unwrap();
                });

                self.textbox.use_navigation_avatar(game_io);
                self.textbox.push_interface(interface);
                self.textbox.open();
            }
            return;
        }

        // handle moving cursor
        let prev_index = self.scroll_tracker.selected_index();

        self.scroll_tracker
            .handle_vertical_input(&self.ui_input_tracker);

        let index = self.scroll_tracker.selected_index();

        // handle attempting to organize a package before the default package
        if self.moving_package && index == 0 {
            globals.audio.play_sound(&globals.sfx.cursor_error);
            self.scroll_tracker.set_selected_index(1);
            return;
        }

        // play a sound for moving the cursor
        if prev_index != index {
            globals.audio.play_sound(&globals.sfx.cursor_move);

            // handling moving packages
            if self.moving_package {
                let moved_package = self.package_order.remove(prev_index);
                self.package_order.insert(index, moved_package);
            }
        }

        // handle opening the context menu
        if self.ui_input_tracker.pulsed(Input::Option) {
            globals.audio.play_sound(&globals.sfx.cursor_select);

            if index == 0 {
                let options = &[("config-resource-order-option-view", MenuOption::View)];
                self.context_menu
                    .set_and_translate_options(game_io, options);
            } else {
                let options = &[
                    ("config-resource-order-option-move", MenuOption::Move),
                    ("config-resource-order-option-view", MenuOption::View),
                ];
                self.context_menu
                    .set_and_translate_options(game_io, options);
            }

            self.moving_package = false;
            self.context_menu.open();
            return;
        }

        // handle selection
        if self.ui_input_tracker.pulsed(Input::Confirm) {
            if index == 0 {
                globals.audio.play_sound(&globals.sfx.cursor_error);
            } else {
                globals.audio.play_sound(&globals.sfx.cursor_select);

                if self.moving_package {
                    // stop moving packages
                    self.moving_package = false;
                } else {
                    // enable package
                    let (_, enabled) = &mut self.package_order[index];
                    *enabled = !*enabled;
                }
            }
        }
    }

    fn update_context_menu(&mut self, game_io: &mut GameIO) {
        let Some(option) = self.context_menu.update(game_io, &self.ui_input_tracker) else {
            return;
        };

        match option {
            MenuOption::View => {
                let index = self.scroll_tracker.selected_index();
                let (package_listing, _) = &self.package_order[index];

                let scene = PackageScene::new(game_io, package_listing.clone());
                let transition = crate::transitions::new_sub_scene(game_io);

                self.next_scene = NextScene::new_push(scene).with_transition(transition);
            }
            MenuOption::Move => {
                self.moving_package = !self.moving_package;
                self.context_menu.close();
            }
        }
    }

    fn handle_events(&mut self, game_io: &mut GameIO) {
        let Ok(event) = self.event_receiver.try_recv() else {
            return;
        };

        match event {
            Event::SaveResponse { save } => {
                if save {
                    let globals = Globals::from_resources_mut(game_io);

                    globals.global_save.resource_package_order = self.collect_order();
                    globals.global_save.resource_order_time = GlobalSave::current_time();
                    globals.global_save.save();

                    let event_sender = self.event_sender.clone();
                    let message = globals.translate("config-resource-order-save-complete");

                    let interface = TextboxMessage::new(message).with_callback(move || {
                        event_sender.send(Event::Exit).unwrap();
                    });
                    self.textbox.push_interface(interface);
                } else {
                    self.exit(game_io);
                }
            }
            Event::Exit => {
                self.exit(game_io);
            }
        }
    }

    fn collect_order(&self) -> Vec<(PackageId, bool)> {
        self.package_order
            .iter()
            .skip(1)
            .map(|(listing, enabled)| (listing.id.clone(), *enabled))
            .collect()
    }

    fn exit(&mut self, game_io: &GameIO) {
        let transition = crate::transitions::new_sub_scene_pop(game_io);
        self.next_scene = NextScene::new_pop().with_transition(transition);
    }
}

impl Scene for ResourceOrderScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        self.textbox.update(game_io);

        if !game_io.is_in_transition() {
            self.handle_input(game_io);
        }

        self.handle_events(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw bg
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw frame
        self.scrollable_frame.draw(game_io, &mut sprite_queue);

        // draw items
        let mut text_style = TextStyle::new_monospace(game_io, FontName::Thick).with_ellipsis(true);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;

        let start_position = self.list_bounds.top_left();
        text_style.bounds.set_position(start_position);
        text_style.bounds.height = text_style.line_height();

        let longest_status_width = (text_style.measure(&self.enabled_string).size.x)
            .max(text_style.measure(&self.disabled_string).size.x);
        let name_available_width =
            self.list_bounds.width - text_style.measure_grapheme(" ").x - longest_status_width;

        for &(ref package_listing, enabled) in &self.package_order[self.scroll_tracker.view_range()]
        {
            text_style.color = if enabled {
                Color::WHITE
            } else {
                DIM_TEXT_COLOR
            };

            // draw package name
            text_style.bounds.width = name_available_width;
            text_style.draw(game_io, &mut sprite_queue, &package_listing.name);

            text_style.bounds.width = f32::INFINITY;

            // draw status
            let status_text = if enabled {
                &self.enabled_string
            } else {
                &self.disabled_string
            };

            text_style.bounds.x = self.list_bounds.right() - text_style.measure(status_text).size.x;
            text_style.draw(game_io, &mut sprite_queue, status_text);

            // update position for next iteration
            text_style.bounds.x = start_position.x;
            text_style.bounds.y += self.scroll_tracker.cursor_multiplier();
        }

        // draw cursor + scrollbar
        self.scroll_tracker.draw_cursor(&mut sprite_queue);
        self.scroll_tracker.draw_scrollbar(&mut sprite_queue);

        // draw frame
        self.frame.draw(&mut sprite_queue);

        // draw option tip
        self.input_tip.draw(game_io, &mut sprite_queue);

        // draw title
        self.scene_title.draw(game_io, &mut sprite_queue);

        // draw context menu
        self.context_menu.draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
