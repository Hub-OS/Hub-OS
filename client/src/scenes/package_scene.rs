use crate::bindable::SpriteColorMode;
use crate::packages::{PackageNamespace, RepoPackageUpdater, UpdateStatus};
use crate::render::ui::{
    build_9patch, FontStyle, PackageListing, PackagePreview, PackagePreviewData, SceneTitle,
    ScrollableList, SubSceneFrame, Text, TextStyle, Textbox, TextboxDoorstop,
    TextboxDoorstopRemover, TextboxMessage, UiButton, UiInputTracker, UiLayout, UiLayoutNode,
    UiNode, UiStyle,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, Input, InputUtil, LocalAssetManager, ResourcePaths};
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use taffy::style::{AlignItems, Dimension, FlexDirection};

enum Event {
    ReceivedAuthor(String),
    StartDownload,
    Delete,
}

pub struct PackageScene {
    camera: Camera,
    background: Background,
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
    doorstop_remover: Option<TextboxDoorstopRemover>,
    package_updater: RepoPackageUpdater,
    prev_status: UpdateStatus,
    textbox: Textbox,
    next_scene: NextScene,
}

impl PackageScene {
    pub fn new(game_io: &GameIO, listing: PackageListing) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // layout
        let mut animator = Animator::load_new(assets, ResourcePaths::PACKAGE_LAYOUT_ANIMATION);
        animator.set_state("DEFAULT");

        let preview_position = animator.point("PREVIEW").unwrap_or_default();

        let list_bounds = Rect::from_corners(
            animator.point("LIST_START").unwrap_or_default(),
            animator.point("LIST_END").unwrap_or_default(),
        );

        let button_bounds = Rect::from_corners(
            animator.point("BUTTONS_START").unwrap_or_default(),
            animator.point("BUTTONS_END").unwrap_or_default(),
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
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_arms(true),
            ui_input_tracker: UiInputTracker::new(),
            preview: PackagePreview::new(listing).with_position(preview_position),
            list: ScrollableList::new(game_io, list_bounds, 15.0).with_focus(false),
            buttons,
            cursor_sprite,
            cursor_animator,
            event_sender,
            event_receiver,
            tasks: Vec::new(),
            doorstop_remover: None,
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
        author: String,
    ) -> Vec<Box<dyn UiNode>> {
        let mut style = TextStyle::new(game_io, FontStyle::Thin);
        style.bounds = list.list_bounds();

        let push_text = |children: &mut Vec<Box<dyn UiNode + 'static>>, text: &str| {
            let ranges = style.measure(text).line_ranges;

            for range in ranges {
                children.push(Box::new(
                    Text::new(game_io, FontStyle::Thin).with_str(&text[range]),
                ));
            }
        };

        let push_blank = |children: &mut Vec<Box<dyn UiNode + 'static>>| {
            children.push(Box::new(()));
        };

        let mut children: Vec<Box<dyn UiNode>> = Vec::new();
        push_text(&mut children, &listing.name);

        #[allow(clippy::single_match)]
        match &listing.preview_data {
            PackagePreviewData::Card { codes, .. } => {
                if !codes.is_empty() {
                    push_blank(&mut children);
                    push_text(&mut children, &format!("Codes: {}", codes.join(" ")));
                }
            }
            _ => {}
        }

        if !listing.description.is_empty() {
            push_blank(&mut children);
            push_text(&mut children, &listing.description);
        }

        push_blank(&mut children);
        push_text(&mut children, &format!("Author: {author}"));

        children
    }

    fn create_buttons(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
        bounds: Rect,
        listing: &PackageListing,
    ) -> UiLayout {
        let globals = game_io.resource::<Globals>().unwrap();
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
                UiButton::new_text(game_io, FontStyle::Thick, "Update").on_activate({
                    let sender = event_sender.clone();

                    move || sender.send(Event::StartDownload).unwrap()
                }),
                UiButton::new_text(game_io, FontStyle::Thick, "Delete").on_activate({
                    let sender = event_sender.clone();

                    move || sender.send(Event::Delete).unwrap()
                }),
            ]
        } else {
            vec![
                UiButton::new_text(game_io, FontStyle::Thick, "Install").on_activate({
                    let sender = event_sender.clone();

                    move || sender.send(Event::StartDownload).unwrap()
                }),
            ]
        };

        let ui_texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let ui_animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);
        let button_9patch = build_9patch!(game_io, ui_texture, &ui_animator, "BUTTON");

        let button_style = UiStyle {
            margin_top: Dimension::Auto,
            margin_right: Dimension::Points(2.0),
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
            align_items: AlignItems::FlexEnd,
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

        if self.ui_input_tracker.is_active(Input::Up) {
            self.list
                .set_selected_index(self.list.top_index().max(1) - 1);
        }

        if self.ui_input_tracker.is_active(Input::Down) {
            let len = self.list.total_children();
            let page_end = self.list.top_index() + self.list.view_size();
            let end_index = len.min(page_end);

            self.list.set_selected_index(end_index);
        }

        if self.ui_input_tracker.is_active(Input::ShoulderL) {
            self.list.page_up();
        }

        if self.ui_input_tracker.is_active(Input::ShoulderR) {
            self.list.page_down();
        }

        if prev_top_index != self.list.top_index() {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        self.buttons.update(game_io, &self.ui_input_tracker);

        // leave scene
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let transition = crate::transitions::new_sub_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);

            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.menu_close);
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
            Event::ReceivedAuthor(author) => {
                self.list.set_children(Self::generate_list(
                    game_io,
                    &self.list,
                    self.preview.listing(),
                    author,
                ));
            }
            Event::StartDownload => {
                let listing = self.preview.listing();
                self.package_updater.begin(game_io, [listing.id.clone()]);
            }
            Event::Delete => {
                let globals = game_io.resource::<Globals>().unwrap();

                if globals.connected_to_server {
                    let interface = TextboxMessage::new(String::from(
                        "Package can't be deleted while connected to a server.",
                    ));

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                } else {
                    self.delete_package(game_io);
                    self.reload_buttons(game_io);

                    // update player avatar
                    self.textbox.use_player_avatar(game_io);

                    let interface = TextboxMessage::new(String::from("Package deleted."));
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

        if let Some(doorstop_remover) = self.doorstop_remover.take() {
            doorstop_remover();
        }

        let message = match status {
            UpdateStatus::Idle => unreachable!(),
            UpdateStatus::CheckingForUpdate => "Checking for updates...",
            UpdateStatus::DownloadingPackage => {
                if self.package_updater.processed_packages() == 0 {
                    "Downloading package..."
                } else {
                    "Downloading dependency..."
                }
            }
            UpdateStatus::Failed => "Download failed.",
            UpdateStatus::Success => {
                if self.package_updater.total_updated() == 0 {
                    "Already up to date."
                } else {
                    "Download successful."
                }
            }
        };

        if matches!(status, UpdateStatus::Failed | UpdateStatus::Success) {
            // clear cached assets
            let assets = LocalAssetManager::new(game_io);
            let globals = game_io.resource_mut::<Globals>().unwrap();
            globals.assets = assets;

            // update player avatar
            self.textbox.use_player_avatar(game_io);

            // reload ui
            self.reload_buttons(game_io);

            // notify player
            let interface = TextboxMessage::new(String::from(message));
            self.textbox.push_interface(interface);
        } else {
            let (doorstop, doorstop_remover) = TextboxDoorstop::new();
            self.doorstop_remover = Some(doorstop_remover);
            self.textbox.push_interface(doorstop.with_str(message));
        }

        self.textbox.open();
    }

    fn request_author(&mut self, game_io: &mut GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        let repo = &globals.config.package_repo;
        let listing = self.preview.listing();
        let encoded_id = uri_encode(&listing.creator);
        let uri = format!("{repo}/api/users/{encoded_id}");

        let task = game_io.spawn_local_task(async move {
            let Some(value) = crate::http::request_json(&uri).await else {
                // provide default
                return Event::ReceivedAuthor(String::from("unknown"));
            };

            let author = value
                .get("username")
                .and_then(|v| v.as_str())
                .unwrap_or_default();

            Event::ReceivedAuthor(author.to_string())
        });

        self.tasks.push(task);
    }

    fn delete_package(&mut self, game_io: &mut GameIO) {
        let listing = self.preview.listing();

        let id = &listing.id;

        let Some(category) = listing.preview_data.category() else {
            return;
        };

        let globals = game_io.resource_mut::<Globals>().unwrap();
        let path = globals.resolve_package_download_path(category, id);

        globals.unload_package(category, PackageNamespace::Local, id);

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
            self.request_author(game_io);
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
        SceneTitle::new("PACKAGE INFO").draw(game_io, &mut sprite_queue);

        self.preview.draw(game_io, &mut sprite_queue);
        self.list.draw(game_io, &mut sprite_queue);
        self.buttons.draw(game_io, &mut sprite_queue);
        sprite_queue.draw_sprite(&self.cursor_sprite);

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
