use crate::bindable::SpriteColorMode;
use crate::packages::{PackageNamespace, RepoPackageUpdater, UpdateStatus};
use crate::render::ui::*;
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::{
    AssetManager, Globals, Input, InputUtil, ResourcePaths, TEXT_DARK_SHADOW_COLOR,
};
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use std::rc::Rc;
use taffy::style::{AlignItems, Dimension, FlexDirection};

enum Event {
    ReceivedUploader(String),
    StartDownload,
    Delete,
}

pub struct PackageScene {
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
    frame: SubSceneFrame,
    ui_input_tracker: UiInputTracker,
    preview: PackagePreview,
    list: ScrollableList,
    buttons: UiLayout,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    tasks: Vec<AsyncTask<Event>>,
    doorstop_key: Option<TextboxDoorstopKey>,
    package_updater: RepoPackageUpdater,
    prev_status: UpdateStatus,
    textbox: Textbox,
    next_scene: NextScene,
}

impl PackageScene {
    pub fn new(game_io: &GameIO, listing: Rc<PackageListing>) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        // layout
        let mut animator = Animator::load_new(assets, ResourcePaths::PACKAGE_UI_ANIMATION);
        animator.set_state("DEFAULT");

        let preview_position = animator.point_or_zero("PREVIEW");

        let list_bounds = Rect::from_corners(
            animator.point_or_zero("LIST_START"),
            animator.point_or_zero("LIST_END"),
        );

        let button_bounds = Rect::from_corners(
            animator.point_or_zero("BUTTONS_START"),
            animator.point_or_zero("BUTTONS_END"),
        );

        // buttons
        let (event_sender, event_receiver) = flume::unbounded();

        let buttons = Self::create_buttons(game_io, &event_sender, button_bounds, &listing);

        // cursor
        let cursor_sprite = assets.new_sprite(game_io, ResourcePaths::SELECT_CURSOR);

        let mut cursor_animator =
            Animator::load_new(assets, ResourcePaths::SELECT_CURSOR_ANIMATION);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            scene_title: SceneTitle::new(game_io, "package-info-scene-title"),
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_arms(true),
            ui_input_tracker: UiInputTracker::new(),
            preview: PackagePreview::new(game_io, listing).with_position(preview_position),
            list: ScrollableList::new(game_io, list_bounds, 15.0).with_focus(false),
            buttons,
            cursor_sprite,
            cursor_animator,
            event_sender,
            event_receiver,
            tasks: Vec::new(),
            doorstop_key: None,
            package_updater: RepoPackageUpdater::new(),
            prev_status: UpdateStatus::Idle,
            textbox: Textbox::new_navigation(game_io),
            next_scene: NextScene::None,
        }
    }

    fn generate_list(
        game_io: &GameIO,
        list: &ScrollableList,
        listing: &PackageListing,
        uploader: &str,
    ) -> Vec<Box<dyn UiNode>> {
        let mut style = TextStyle::new(game_io, FontName::Thin);
        style.bounds = list.list_bounds();
        style.bounds.height = f32::INFINITY;

        let push_text = |children: &mut Vec<Box<dyn UiNode + 'static>>, text: &str| {
            let ranges = style.measure(text).line_ranges;

            for range in ranges {
                children.push(Box::new(
                    Text::new(game_io, FontName::Thin)
                        .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
                        .with_str(&text[range]),
                ));
            }
        };

        let push_blank = |children: &mut Vec<Box<dyn UiNode + 'static>>| {
            children.push(Box::new(()));
        };

        let mut children: Vec<Box<dyn UiNode>> = Vec::new();
        push_text(&mut children, &listing.long_name);

        if let PackagePreviewData::Card { codes, .. } = &listing.preview_data
            && !codes.is_empty()
        {
            push_blank(&mut children);
            push_text(&mut children, &format!("Codes: {}", codes.join(" ")));
        }

        if !listing.description.is_empty() {
            push_blank(&mut children);
            push_text(&mut children, &listing.description);
        }

        if !uploader.is_empty() {
            push_blank(&mut children);
            push_text(&mut children, &format!("Uploader: {uploader}"));
        }

        push_blank(&mut children);
        push_text(&mut children, &format!("Package ID: {}", listing.id));

        children
    }

    fn create_buttons(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
        bounds: Rect,
        listing: &PackageListing,
    ) -> UiLayout {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let installed = if let Some(category) = listing.preview_data.category() {
            globals
                .package_info(category, PackageNamespace::Local, &listing.id)
                .is_some()
        } else {
            false
        };

        let buttons = if installed {
            vec![
                UiButton::new_translated(game_io, FontName::Thick, "package-info-button-update")
                    .on_activate({
                        let sender = event_sender.clone();

                        move || sender.send(Event::StartDownload).unwrap()
                    }),
                UiButton::new_translated(game_io, FontName::Thick, "package-info-button-delete")
                    .on_activate({
                        let sender = event_sender.clone();

                        move || sender.send(Event::Delete).unwrap()
                    }),
            ]
        } else {
            vec![
                UiButton::new_translated(game_io, FontName::Thick, "package-info-button-install")
                    .on_activate({
                        let sender = event_sender.clone();

                        move || sender.send(Event::StartDownload).unwrap()
                    }),
            ]
        };

        let ui_texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let ui_animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);
        let button_9patch = build_9patch!(game_io, ui_texture, &ui_animator, "BUTTON");

        let button_style = UiStyle {
            margin_top: LengthPercentageAuto::Auto,
            margin_right: LengthPercentageAuto::Points(2.0),
            nine_patch: Some(button_9patch),
            ..Default::default()
        };

        UiLayout::new_horizontal(
            bounds,
            buttons
                .into_iter()
                .map(|button| UiLayoutNode::new(button).with_style(button_style.clone()))
                .collect(),
        )
        .with_style(UiStyle {
            // flex_direction: FlexDirection::RowReverse,
            flex_direction: FlexDirection::Row,
            flex_grow: 1.0,
            align_items: Some(AlignItems::FlexEnd),
            min_width: Dimension::Points(bounds.width),
            min_height: Dimension::Points(bounds.height),
            ..Default::default()
        })
    }

    fn reload_buttons(&mut self, game_io: &GameIO) {
        let event_sender = &self.event_sender;
        let bounds = self.buttons.bounds();
        let listing = self.preview.listing();
        self.buttons = Self::create_buttons(game_io, event_sender, bounds, listing);
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

        // scroll
        self.list.update(game_io, &self.ui_input_tracker);

        let prev_top_index = self.list.top_index();

        if self.ui_input_tracker.pulsed(Input::Up) {
            self.list
                .set_selected_index(self.list.top_index().max(1) - 1);
        }

        if self.ui_input_tracker.pulsed(Input::Down) {
            let len = self.list.total_children();
            let page_end = self.list.top_index() + self.list.view_size();
            let end_index = len.min(page_end);

            self.list.set_selected_index(end_index);
        }

        if self.ui_input_tracker.pulsed(Input::ShoulderL) {
            self.list.page_up();
        }

        if self.ui_input_tracker.pulsed(Input::ShoulderR) {
            self.list.page_down();
        }

        if prev_top_index != self.list.top_index() {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        self.buttons.update(game_io, &self.ui_input_tracker);

        // leave scene
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let transition = crate::transitions::new_sub_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);

            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_cancel);
        }
    }

    fn handle_events(&mut self, game_io: &mut GameIO) {
        let mut completed_tasks = Vec::new();

        for (i, task) in self.tasks.iter().enumerate() {
            if task.is_finished() {
                completed_tasks.push(i);
            }
        }

        completed_tasks.reverse();

        for i in completed_tasks {
            let task = self.tasks.remove(i);
            let event = task.join().unwrap();
            self.handle_event(game_io, event);
        }

        while let Ok(event) = self.event_receiver.try_recv() {
            self.handle_event(game_io, event);
        }
    }

    fn handle_event(&mut self, game_io: &mut GameIO, event: Event) {
        match event {
            Event::ReceivedUploader(uploader) => {
                self.list.set_children(Self::generate_list(
                    game_io,
                    &self.list,
                    self.preview.listing(),
                    &uploader,
                ));
            }
            Event::StartDownload => {
                let listing = self.preview.listing();
                self.package_updater.begin(game_io, [listing.id.clone()]);
            }
            Event::Delete => {
                let globals = Globals::from_resources(game_io);

                if globals.connected_to_server {
                    let interface = TextboxMessage::new(
                        globals.translate("package-info-delete-while-connected-error"),
                    );

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                } else {
                    let message = globals.translate("package-info-delete-success");

                    self.delete_package(game_io);
                    self.reload_buttons(game_io);

                    // update player avatar
                    self.textbox.use_navigation_avatar(game_io);

                    let interface = TextboxMessage::new(message);
                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
            }
        }
    }

    fn handle_updater(&mut self, game_io: &mut GameIO) {
        self.package_updater.update(game_io);
        let status = self.package_updater.status();

        if status == self.prev_status {
            return;
        }
        self.prev_status = status;

        self.doorstop_key.take();

        let globals = Globals::from_resources(game_io);

        let message_key = match status {
            UpdateStatus::Idle => unreachable!(),
            UpdateStatus::CheckingForUpdate => "package-info-checking-for-updates",
            UpdateStatus::DownloadingPackage => "package-info-downloading",
            UpdateStatus::Failed => "package-info-update-error",
            UpdateStatus::Success => {
                if self.package_updater.total_updated() == 0 {
                    "package-info-update-unnecessary"
                } else {
                    "package-info-update-success"
                }
            }
        };

        let message = globals.translate(message_key);

        if matches!(status, UpdateStatus::Failed | UpdateStatus::Success) {
            // clear cached assets
            globals.assets.clear_local_mod_assets();

            // update player avatar
            self.textbox.use_navigation_avatar(game_io);

            // reload ui
            self.reload_buttons(game_io);

            if self.preview.listing().local && self.package_updater.total_updated() > 0 {
                // reload preview
                let prev_listing = self.preview.listing();
                let category = prev_listing.preview_data.category();

                if let Some(new_listing) = category
                    .and_then(|category| globals.create_package_listing(category, &prev_listing.id))
                {
                    let position = self.preview.position();
                    self.preview =
                        PackagePreview::new(game_io, new_listing.into()).with_position(position);
                }

                // update list
                self.list.set_children(Self::generate_list(
                    game_io,
                    &self.list,
                    self.preview.listing(),
                    "",
                ));
            }

            // notify player
            let interface = TextboxMessage::new(message);
            self.textbox.push_interface(interface);
        } else {
            let (doorstop, doorstop_key) = TextboxDoorstop::new();
            self.doorstop_key = Some(doorstop_key);
            self.textbox.push_interface(doorstop.with_string(message));
        }

        self.textbox.open();
    }

    fn request_uploader(&mut self, game_io: &GameIO) {
        let listing = self.preview.listing();

        if listing.creator.is_empty() {
            let event = Event::ReceivedUploader(Default::default());
            let _ = self.event_sender.send(event);
            return;
        }

        let globals = Globals::from_resources(game_io);

        let repo = &globals.config.package_repo;
        let encoded_id = uri_encode(&listing.creator);
        let uri = format!("{repo}/api/users/{encoded_id}");

        let task = game_io.spawn_local_task(async move {
            let Some(value) = crate::http::request_json(&uri).await else {
                // provide default
                return Event::ReceivedUploader(Default::default());
            };

            let uploader = value
                .get("username")
                .and_then(|v| v.as_str())
                .unwrap_or_default();

            Event::ReceivedUploader(uploader.to_string())
        });

        self.tasks.push(task);
    }

    fn delete_package(&mut self, game_io: &mut GameIO) {
        let listing = self.preview.listing();

        let id = &listing.id;

        let Some(category) = listing.preview_data.category() else {
            return;
        };

        let globals = Globals::from_resources_mut(game_io);
        let path = globals.resolve_package_download_path(category, id);

        globals.unload_package(category, PackageNamespace::Local, id);

        // remove associated save data
        globals.global_save.remove_package_id(id);
        globals.global_save.save();

        log::info!("Deleting {path:?}");
        let _ = std::fs::remove_dir_all(path);
    }

    fn update_cursor(&mut self) {
        self.cursor_animator.update();
        self.cursor_animator.apply(&mut self.cursor_sprite);

        let index = self.buttons.focused_index().unwrap();
        let bounds = self.buttons.get_bounds(index).unwrap();
        let position = bounds.center_left() - self.cursor_sprite.size() * 0.5;

        self.cursor_sprite.set_position(position);
    }
}

impl Scene for PackageScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        if self.list.total_children() == 0 {
            self.request_uploader(game_io);
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.preview.update(game_io);

        self.handle_input(game_io);
        self.handle_updater(game_io);
        self.handle_events(game_io);
        self.update_cursor();
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        self.frame.draw(&mut sprite_queue);
        self.scene_title.draw(game_io, &mut sprite_queue);

        self.preview.draw(game_io, &mut sprite_queue);
        self.list.draw(game_io, &mut sprite_queue);
        self.buttons.draw(game_io, &mut sprite_queue);
        sprite_queue.draw_sprite(&self.cursor_sprite);

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
