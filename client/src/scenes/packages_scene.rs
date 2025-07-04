use super::PackageScene;
use crate::bindable::SpriteColorMode;
use crate::render::ui::{
    build_9patch, ContextMenu, FontName, LengthPercentageAuto, PackageListing, SceneTitle,
    ScrollableList, SubSceneFrame, Textbox, TextboxPrompt, UiButton, UiInputTracker, UiLayout,
    UiLayoutNode, UiNode, UiStyle,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, Input, InputUtil, ResourcePaths};
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use strum::{EnumIter, IntoEnumIterator};
use taffy::style::{Dimension, FlexDirection};

const DIM_COLOR: Color = Color::new(0.75, 0.75, 0.75, 1.0);
const PACKAGES_PER_REQUEST: usize = 21;

type RequestTask = AsyncTask<Vec<PackageListing>>;

enum Event {
    OpenSearch,
    OpenCategoryMenu,
    FilterName(String),
    ViewPackage { listing: PackageListing },
}

#[derive(Default, EnumIter, Clone, Copy, PartialEq, Eq)]
pub enum CategoryFilter {
    #[default]
    All,
    Cards,
    Augments,
    Encounters,
    Players,
    Resource,
    Packs,
}

impl CategoryFilter {
    fn name(&self) -> &'static str {
        match self {
            CategoryFilter::All => "All",
            CategoryFilter::Cards => "Cards",
            CategoryFilter::Augments => "Augments",
            CategoryFilter::Encounters => "Battles",
            CategoryFilter::Players => "Players",
            CategoryFilter::Resource => "Resource",
            CategoryFilter::Packs => "Packs",
        }
    }

    fn value(&self) -> &'static str {
        match self {
            CategoryFilter::All => "",
            CategoryFilter::Cards => "card",
            CategoryFilter::Augments => "augment",
            CategoryFilter::Encounters => "encounter",
            CategoryFilter::Players => "player",
            CategoryFilter::Resource => "resource",
            CategoryFilter::Packs => "pack",
        }
    }
}

pub struct PackagesScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    ui_input_tracker: UiInputTracker,
    sidebar: UiLayout,
    category_menu: ContextMenu<CategoryFilter>,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    list: ScrollableList,
    list_task: Option<RequestTask>,
    exhausted_list: bool,
    category_filter: CategoryFilter,
    name_filter: String,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    textbox: Textbox,
    next_scene: NextScene,
}

impl PackagesScene {
    pub fn new(game_io: &GameIO, initial_category: CategoryFilter) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // cursor
        let cursor_sprite = assets.new_sprite(game_io, ResourcePaths::SELECT_CURSOR);

        let mut cursor_animator =
            Animator::load_new(assets, ResourcePaths::SELECT_CURSOR_ANIMATION);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        // layout
        let mut animator = Animator::load_new(assets, ResourcePaths::PACKAGES_UI_ANIMATION);
        animator.set_state("DEFAULT");

        let sidebar_bounds = Rect::from_corners(
            animator.point_or_zero("SIDEBAR_START"),
            animator.point_or_zero("SIDEBAR_END"),
        );

        let list_bounds = Rect::from_corners(
            animator.point_or_zero("LIST_START"),
            animator.point_or_zero("LIST_END"),
        );

        let context_menu_position = animator.point_or_zero("CONTEXT_MENU");

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        let mut scene = Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_arms(true),
            ui_input_tracker: UiInputTracker::new(),
            sidebar: Self::generate_sidebar(game_io, event_sender.clone(), sidebar_bounds),
            category_menu: ContextMenu::new(game_io, "Category", context_menu_position)
                .with_options(
                    game_io,
                    CategoryFilter::iter().map(|filter| (filter.name(), filter)),
                ),
            cursor_sprite,
            cursor_animator,
            list: ScrollableList::new(game_io, list_bounds, 15.0).with_focus(false),
            list_task: None,
            exhausted_list: false,
            category_filter: initial_category,
            name_filter: String::new(),
            event_sender,
            event_receiver,
            textbox: Textbox::new_navigation(game_io),
            next_scene: NextScene::None,
        };

        scene.request_more(game_io, 0);

        scene
    }

    fn request_more(&mut self, game_io: &GameIO, skip: usize) {
        let globals = game_io.resource::<Globals>().unwrap();
        let repo = globals.config.package_repo.clone();

        let mut uri = format!("{repo}/api/mods?skip={skip}&limit={PACKAGES_PER_REQUEST}");

        if self.category_filter != CategoryFilter::All {
            uri += &format!("&category={}", self.category_filter.value());
        }

        if !self.name_filter.is_empty() {
            uri += &format!("&name={}", uri_encode(&self.name_filter));
        }

        let task = game_io.spawn_local_task(async move {
            let Some(value) = crate::http::request_json(&uri).await else {
                return Vec::new();
            };

            let Some(list) = value.as_array() else {
                return Vec::new();
            };

            list.iter().map(|v| v.into()).collect()
        });

        self.list_task = Some(task);
        self.list
            .set_label(self.category_filter.name().to_uppercase());
    }

    fn generate_sidebar(
        game_io: &GameIO,
        event_sender: flume::Sender<Event>,
        sidebar_bounds: Rect,
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
            sidebar_bounds,
            [
                UiButton::new_text(game_io, FontName::Thick, "Search").on_activate({
                    let event_sender = event_sender.clone();

                    move || {
                        event_sender.send(Event::OpenSearch).unwrap();
                    }
                }),
                UiButton::new_text(game_io, FontName::Thick, "Category").on_activate({
                    let event_sender = event_sender.clone();

                    move || {
                        event_sender.send(Event::OpenCategoryMenu).unwrap();
                    }
                }),
            ]
            .into_iter()
            .map(|node: UiButton<'static, _>| {
                UiLayoutNode::new(node).with_style(option_style.clone())
            })
            .collect(),
        )
        .with_style(UiStyle {
            flex_direction: FlexDirection::Column,
            min_width: Dimension::Points(sidebar_bounds.width),
            ..Default::default()
        })
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        self.ui_input_tracker.update(game_io);

        // update textbox
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

        // update list
        if !self.sidebar.focused() {
            self.list.update(game_io, &self.ui_input_tracker);
        }

        let input_util = InputUtil::new(game_io);

        // switch between sidebar and list with left + right
        if input_util.was_just_pressed(Input::Left) && self.list.focused() {
            self.sidebar.set_focused(true);
            self.list.set_focused(false);
            self.sidebar.focus_default();

            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);
        } else if input_util.was_just_pressed(Input::Right) && self.sidebar.focused() {
            self.sidebar.set_focused(false);
            self.list.set_focused(true);

            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);
        }

        // handle exiting the scene or the list
        if input_util.was_just_pressed(Input::Cancel) {
            if self.sidebar.focused() {
                self.leave(game_io);
            } else {
                self.sidebar.set_focused(true);
                self.list.set_focused(false);
                self.sidebar.focus_default();

                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.sfx.cursor_cancel);
            }
        }

        // update the sidebar
        self.sidebar.update(game_io, &self.ui_input_tracker);

        // update the category menu
        if let Some(selection) = self.category_menu.update(game_io, &self.ui_input_tracker) {
            if selection != self.category_filter {
                self.category_filter = selection;
                self.restart_search(game_io);
            }
            self.category_menu.close();
            self.sidebar.set_focused(true);
        }
    }

    fn leave(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        let transition = crate::transitions::new_sub_scene_pop(game_io);
        self.next_scene = NextScene::new_pop().with_transition(transition);

        globals.audio.play_sound(&globals.sfx.cursor_cancel);
    }

    fn handle_events(&mut self, game_io: &GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::OpenSearch => {
                    let event_sender = self.event_sender.clone();
                    let interface = TextboxPrompt::new(move |name_filter| {
                        event_sender.send(Event::FilterName(name_filter)).unwrap();
                    });

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                Event::OpenCategoryMenu => {
                    self.category_menu.open();
                    self.sidebar.set_focused(false);
                }
                Event::FilterName(name_filter) => {
                    self.name_filter = name_filter;
                    self.restart_search(game_io);
                }
                Event::ViewPackage { listing } => {
                    if !game_io.is_in_transition() {
                        self.next_scene = NextScene::new_push(PackageScene::new(game_io, listing))
                            .with_transition(crate::transitions::new_sub_scene(game_io));
                    }
                }
            }
        }
    }

    fn restart_search(&mut self, game_io: &GameIO) {
        self.list.set_children(vec![]);
        self.exhausted_list = false;
        self.request_more(game_io, 0);
    }

    fn update_cursor(&mut self) {
        self.cursor_animator.update();
        self.cursor_animator.apply(&mut self.cursor_sprite);

        let index = self.sidebar.focused_index().unwrap();
        let bounds = self.sidebar.get_bounds(index).unwrap();
        let position = bounds.center_left() - self.cursor_sprite.size() * 0.5;

        self.cursor_sprite.set_position(position);
    }
}

impl Scene for PackagesScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.textbox.use_navigation_avatar(game_io);
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();

        let finished_request = matches!(&self.list_task, Some(task) if task.is_finished());

        if finished_request {
            let task = self.list_task.take().unwrap();
            let items = task.join().unwrap();

            if items.len() < PACKAGES_PER_REQUEST {
                self.exhausted_list = true;
            }

            self.list.append_children(
                items
                    .into_iter()
                    .map(|listing| -> Box<dyn UiNode> {
                        Box::new(UiButton::new(listing.clone()).on_activate({
                            let event_sender = self.event_sender.clone();
                            move || {
                                let event = Event::ViewPackage {
                                    listing: listing.clone(),
                                };

                                event_sender.send(event).unwrap();
                            }
                        }))
                    })
                    .collect(),
            )
        }

        let index = self.list.selected_index();
        let total_children = self.list.total_children();

        if !self.exhausted_list
            && self.list_task.is_none()
            && total_children - index < PACKAGES_PER_REQUEST
        {
            self.request_more(game_io, total_children);
        }

        self.handle_input(game_io);
        self.handle_events(game_io);
        self.update_cursor();
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        self.frame.draw(&mut sprite_queue);
        SceneTitle::new("DOWNLOAD MODS").draw(game_io, &mut sprite_queue);

        self.sidebar.draw(game_io, &mut sprite_queue);
        self.list.draw(game_io, &mut sprite_queue);

        if self.sidebar.focused() && !self.textbox.is_open() {
            sprite_queue.draw_sprite(&self.cursor_sprite);
        }

        self.category_menu.draw(game_io, &mut sprite_queue);

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
