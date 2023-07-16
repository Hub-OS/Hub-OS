use crate::bindable::SpriteColorMode;
use crate::packages::{PackageNamespace, RepoPackageUpdater, UpdateStatus};
use crate::render::ui::{
    build_9patch, FontStyle, LengthPercentageAuto, PackageListing, SceneTitle, ScrollableList,
    SubSceneFrame, Textbox, TextboxDoorstop, TextboxDoorstopRemover, TextboxMessage, UiButton,
    UiInputTracker, UiLayout, UiLayoutNode, UiNode, UiStyle,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, Input, InputUtil, LocalAssetManager, ResourcePaths};
use framework::prelude::{GameIO, NextScene, Rect, RenderPass, Scene, Sprite};
use packets::structures::{FileHash, PackageCategory, PackageId};
use taffy::style::{AlignItems, Dimension, FlexDirection, JustifyContent};

use super::PackageScene;

enum Event {
    ViewList,
    ViewPackage { listing: Box<PackageListing> },
    Update,
    Leave,
}

pub struct PackageUpdatesScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    ui_input_tracker: UiInputTracker,
    list: ScrollableList,
    buttons: UiLayout,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    package_updater: RepoPackageUpdater,
    prev_status: UpdateStatus,
    requires_update: Vec<(PackageCategory, PackageId, FileHash)>,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    textbox: Textbox,
    doorstop_remover: Option<TextboxDoorstopRemover>,
    next_scene: NextScene,
}

impl PackageUpdatesScene {
    pub fn new(
        game_io: &GameIO,
        requires_update: Vec<(PackageCategory, PackageId, FileHash)>,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // layout
        let mut layout_animator =
            Animator::load_new(assets, ResourcePaths::PACKAGE_UPDATES_LAYOUT_ANIMATION);
        layout_animator.set_state("DEFAULT");

        let list_bounds = Rect::from_corners(
            layout_animator.point("LIST_START").unwrap_or_default(),
            layout_animator.point("LIST_END").unwrap_or_default(),
        );

        let button_bounds = Rect::from_corners(
            layout_animator.point("BUTTONS_START").unwrap_or_default(),
            layout_animator.point("BUTTONS_END").unwrap_or_default(),
        );

        // cursor
        let cursor_sprite = assets.new_sprite(game_io, ResourcePaths::SELECT_CURSOR);

        let mut cursor_animator =
            Animator::load_new(assets, ResourcePaths::SELECT_CURSOR_ANIMATION);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        // input
        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_arms(true),
            ui_input_tracker: UiInputTracker::new(),
            list: ScrollableList::new(game_io, list_bounds, 15.0).with_focus(false),
            buttons: Self::create_buttons(game_io, &event_sender, button_bounds),
            cursor_sprite,
            cursor_animator,
            package_updater: RepoPackageUpdater::new(),
            prev_status: UpdateStatus::Idle,
            requires_update,
            event_sender,
            event_receiver,
            textbox: Textbox::new_navigation(game_io),
            doorstop_remover: None,
            next_scene: NextScene::None,
        }
    }

    fn generate_list(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
        requires_update: &[(PackageCategory, PackageId, FileHash)],
    ) -> Vec<Box<dyn UiNode>> {
        let globals = game_io.resource::<Globals>().unwrap();

        requires_update
            .iter()
            .flat_map(|(category, id, _)| globals.create_package_listing(*category, id))
            .map(|listing| -> Box<dyn UiNode> {
                Box::new(UiButton::new(listing.clone()).on_activate({
                    let event_sender = event_sender.clone();

                    move || {
                        let event = Event::ViewPackage {
                            listing: Box::new(listing.clone()),
                        };

                        event_sender.send(event).unwrap();
                    }
                }))
            })
            .collect()
    }

    fn create_buttons(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
        bounds: Rect,
    ) -> UiLayout {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

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
            [
                UiButton::new_text(game_io, FontStyle::Thick, "View").on_activate({
                    let sender = event_sender.clone();

                    move || sender.send(Event::ViewList).unwrap()
                }),
                UiButton::new_text(game_io, FontStyle::Thick, "Update All").on_activate({
                    let sender = event_sender.clone();

                    move || sender.send(Event::Update).unwrap()
                }),
            ]
            .into_iter()
            .map(|button| UiLayoutNode::new(button).with_style(button_style.clone()))
            .collect(),
        )
        .with_style(UiStyle {
            flex_direction: FlexDirection::Row,
            flex_grow: 1.0,
            justify_content: Some(JustifyContent::SpaceEvenly),
            align_items: Some(AlignItems::FlexEnd),
            min_width: Dimension::Points(bounds.width),
            min_height: Dimension::Points(bounds.height),
            ..Default::default()
        })
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        self.ui_input_tracker.update(game_io);
        self.textbox.update(game_io);

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        self.list.update(game_io, &self.ui_input_tracker);

        if self.textbox.is_open() || game_io.is_in_transition() {
            return;
        }

        self.buttons.update(game_io, &self.ui_input_tracker);

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.resource::<Globals>().unwrap();

            if self.list.focused() {
                self.list.set_focused(false);
                self.buttons.set_focused(true);

                globals.audio.play_sound(&globals.sfx.cursor_cancel);
            } else {
                let transition = crate::transitions::new_sub_scene_pop(game_io);
                self.next_scene = NextScene::new_pop().with_transition(transition);

                globals.audio.play_sound(&globals.sfx.menu_close);
            }
        }
    }

    fn handle_events(&mut self, game_io: &mut GameIO) {
        if self.next_scene.is_some() || game_io.is_in_transition() {
            return;
        }

        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::ViewList => {
                    self.list.set_focused(true);
                    self.buttons.set_focused(false);
                }
                Event::ViewPackage { listing } => {
                    self.next_scene = NextScene::new_push(PackageScene::new(game_io, *listing))
                        .with_transition(crate::transitions::new_sub_scene(game_io));
                }
                Event::Update => {
                    let (doorstop, doorstop_remover) = TextboxDoorstop::new();
                    self.doorstop_remover = Some(doorstop_remover);
                    self.textbox
                        .push_interface(doorstop.with_str("Updating..."));
                    self.textbox.open();

                    let ids = std::mem::take(&mut self.requires_update)
                        .into_iter()
                        .map(|(_, id, _)| id);

                    self.package_updater.begin(game_io, ids);
                }
                Event::Leave => {
                    let transition = crate::transitions::new_sub_scene_pop(game_io);
                    self.next_scene = NextScene::new_pop().with_transition(transition);
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

        let message = match status {
            UpdateStatus::Failed => "Updating failed.",
            UpdateStatus::Success => "Updates completed!",
            _ => {
                return;
            }
        };

        if let Some(doorstop_remover) = self.doorstop_remover.take() {
            doorstop_remover();
        }

        // clear cached assets
        let assets = LocalAssetManager::new(game_io);
        let globals = game_io.resource_mut::<Globals>().unwrap();
        globals.assets = assets;

        // update player avatar
        self.textbox.use_player_avatar(game_io);

        // notify player
        let event_sender = self.event_sender.clone();
        let interface = TextboxMessage::new(String::from(message))
            .with_callback(move || event_sender.send(Event::Leave).unwrap());
        self.textbox.push_interface(interface);
        self.textbox.open();
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

impl Scene for PackageUpdatesScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        // update player avatar
        self.textbox.use_player_avatar(game_io);

        // update list in case a package was individually updated or deleted
        let globals = game_io.resource::<Globals>().unwrap();
        self.requires_update = std::mem::take(&mut self.requires_update)
            .into_iter()
            .filter(|(category, id, hash)| {
                let Some(package_info) =
                    globals.package_info(*category, PackageNamespace::Local, id)
                else {
                    // deleted
                    return false;
                };

                package_info.hash != *hash
            })
            .collect();

        let children = Self::generate_list(game_io, &self.event_sender, &self.requires_update);
        self.list.set_children(children);
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.handle_input(game_io);
        self.handle_events(game_io);
        self.handle_updater(game_io);
        self.update_cursor();
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        self.frame.draw(&mut sprite_queue);
        SceneTitle::new("PENDING UPDATES").draw(game_io, &mut sprite_queue);

        self.list.draw(game_io, &mut sprite_queue);
        self.buttons.draw(game_io, &mut sprite_queue);

        if self.buttons.focused() {
            sprite_queue.draw_sprite(&self.cursor_sprite);
        }

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
