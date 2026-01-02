use super::PackageScene;
use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::{
    ContextMenu, FontName, IntoUiLayoutNode, LengthPercentageAuto, PackageListing,
    PackagePreviewData, SceneTitle, ScrollableList, SubSceneFrame, Textbox, TextboxPrompt,
    UiButton, UiInputTracker, UiLayout, UiNode, UiStyle, build_9patch,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, Input, InputUtil, ResourcePaths};
use framework::prelude::*;
use nom::AsChar;
use packets::address_parsing::uri_encode;
use packets::structures::PackageCategory;
use strum::{EnumIter, IntoEnumIterator};
use taffy::style::{Dimension, FlexDirection};

const PACKAGES_PER_REQUEST: usize = 21;

type RequestTask = AsyncTask<Vec<PackageListing>>;

#[derive(Clone)]
enum Event {
    OpenSearch,
    OpenCategoryMenu,
    OpenLocationMenu,
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
    fn translation_key(&self) -> &'static str {
        match self {
            CategoryFilter::All => "packages-category-filter-all",
            CategoryFilter::Cards => "packages-category-filter-cards",
            CategoryFilter::Augments => "packages-category-filter-augments",
            CategoryFilter::Encounters => "packages-category-filter-encounters",
            CategoryFilter::Players => "packages-category-filter-players",
            CategoryFilter::Resource => "packages-category-filter-resource",
            CategoryFilter::Packs => "packages-category-filter-packs",
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

    fn package_category(&self) -> Option<PackageCategory> {
        match self {
            CategoryFilter::All => None,
            CategoryFilter::Cards => Some(PackageCategory::Card),
            CategoryFilter::Augments => Some(PackageCategory::Augment),
            CategoryFilter::Encounters => Some(PackageCategory::Encounter),
            CategoryFilter::Players => Some(PackageCategory::Player),
            CategoryFilter::Resource => Some(PackageCategory::Resource),
            CategoryFilter::Packs => Some(PackageCategory::Library),
        }
    }
}

pub struct PackagesScene {
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
    frame: SubSceneFrame,
    ui_input_tracker: UiInputTracker,
    sidebar: UiLayout,
    category_menu: ContextMenu<CategoryFilter>,
    location_menu: ContextMenu<bool>,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    list: ScrollableList,
    list_task: Option<RequestTask>,
    exhausted_list: bool,
    category_filter: CategoryFilter,
    name_filter: String,
    local_only: bool,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    textbox: Textbox,
    next_scene: NextScene,
}

impl PackagesScene {
    pub fn new(game_io: &GameIO, initial_category: CategoryFilter) -> Self {
        let globals = Globals::from_resources(game_io);
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
            scene_title: SceneTitle::new(game_io, "packages-online-scene-title"),
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_arms(true),
            ui_input_tracker: UiInputTracker::new(),
            sidebar: Self::generate_sidebar(game_io, event_sender.clone(), sidebar_bounds),
            category_menu: ContextMenu::new_translated(
                game_io,
                "packages-category-context-menu-label",
                context_menu_position,
            )
            .with_translated_options(
                game_io,
                &CategoryFilter::iter()
                    .map(|filter| (filter.translation_key(), filter))
                    .collect::<Vec<_>>(),
            ),
            location_menu: ContextMenu::new_translated(
                game_io,
                "packages-location-context-menu-label",
                context_menu_position,
            )
            .with_translated_options(
                game_io,
                &[
                    ("packages-location-online", false),
                    ("packages-location-installed", true),
                ],
            ),
            cursor_sprite,
            cursor_animator,
            list: ScrollableList::new(game_io, list_bounds, 15.0).with_focus(false),
            list_task: None,
            exhausted_list: false,
            category_filter: initial_category,
            name_filter: String::new(),
            local_only: false,
            event_sender,
            event_receiver,
            textbox: Textbox::new_navigation(game_io),
            next_scene: NextScene::None,
        };

        scene.request_more(game_io, 0);

        scene
    }

    fn request_more(&mut self, game_io: &GameIO, skip: usize) {
        if self.local_only {
            if skip == 0 {
                self.request_local(game_io);
            }
        } else {
            self.request_remote(game_io, skip);
        }
    }

    fn request_local(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);
        let category_filter = self.category_filter.package_category();

        let mut new_listings: Vec<_> = globals
            .packages(PackageNamespace::Local)
            .filter(|info| {
                info.category != PackageCategory::Character
                    && category_filter.is_none_or(|c| c == info.category)
            })
            .flat_map(|info| globals.create_package_listing(info.category, &info.id))
            .filter(|listing| {
                self.category_filter != CategoryFilter::Packs
                    || matches!(listing.preview_data, PackagePreviewData::Pack)
            })
            .filter(|listing| {
                case_insensitive_contains(&listing.name, &self.name_filter)
                    || case_insensitive_contains(&listing.long_name, &self.name_filter)
            })
            .collect();

        // sort alphabetically
        new_listings.sort_by(|a, b| (&a.long_name, &a.id).cmp(&(&b.long_name, &b.id)));

        self.list.set_children([]);
        self.append_listings(new_listings);
    }

    fn request_remote(&mut self, game_io: &GameIO, skip: usize) {
        let globals = Globals::from_resources(game_io);
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
        let label = globals.translate(self.category_filter.translation_key());
        self.list.set_label(label.to_uppercase());
    }

    fn append_listings(&mut self, items: impl IntoIterator<Item = PackageListing>) {
        self.list
            .append_children(items.into_iter().map(|listing| -> Box<dyn UiNode> {
                Box::new(UiButton::new(listing.clone()).on_activate({
                    let event_sender = self.event_sender.clone();
                    move || {
                        let event = Event::ViewPackage {
                            listing: listing.clone(),
                        };

                        event_sender.send(event).unwrap();
                    }
                }))
            }))
    }

    fn generate_sidebar(
        game_io: &GameIO,
        event_sender: flume::Sender<Event>,
        sidebar_bounds: Rect,
    ) -> UiLayout {
        let assets = &Globals::from_resources(game_io).assets;
        let ui_texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let ui_animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);
        let button_9patch = build_9patch!(game_io, ui_texture, &ui_animator, "BUTTON");

        let option_style = UiStyle {
            margin_bottom: LengthPercentageAuto::Points(0.0),
            nine_patch: Some(button_9patch),
            ..Default::default()
        };

        let create_button = |label: &str, event: Event| {
            let event_sender = event_sender.clone();

            UiButton::new_translated(game_io, FontName::Thick, label)
                .on_activate({
                    move || {
                        event_sender.send(event.clone()).unwrap();
                    }
                })
                .into_layout_node()
                .with_style(option_style.clone())
        };

        UiLayout::new_vertical(
            sidebar_bounds,
            vec![
                create_button("packages-search-tab", Event::OpenSearch),
                create_button("packages-location-tab", Event::OpenLocationMenu),
                create_button("packages-category-tab", Event::OpenCategoryMenu),
            ],
        )
        .with_style(UiStyle {
            flex_direction: FlexDirection::Column,
            flex_grow: 1.0,
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

            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_cancel);
        } else if input_util.was_just_pressed(Input::Right) && self.sidebar.focused() {
            self.sidebar.set_focused(false);
            self.list.set_focused(true);

            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);
        }

        // handle exiting the scene or the list
        if input_util.was_just_pressed(Input::Cancel) {
            if self.sidebar.focused() {
                self.leave(game_io);
            } else {
                self.sidebar.set_focused(true);
                self.list.set_focused(false);

                let globals = Globals::from_resources(game_io);
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
        } else if let Some(local) = self.location_menu.update(game_io, &self.ui_input_tracker) {
            self.list_task = None;
            self.local_only = local;
            self.restart_search(game_io);

            let title = if local {
                "packages-local-scene-title"
            } else {
                "packages-online-scene-title"
            };
            self.scene_title.set_title(game_io, title, vec![]);

            self.location_menu.close();
            self.sidebar.set_focused(true);
        }
    }

    fn leave(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);

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
                Event::OpenLocationMenu => {
                    self.location_menu.open();
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
        self.list.set_children([]);
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

            self.append_listings(items);
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

        self.scene_title.draw(game_io, &mut sprite_queue);

        self.sidebar.draw(game_io, &mut sprite_queue);
        self.list.draw(game_io, &mut sprite_queue);

        if self.sidebar.focused() && !self.textbox.is_open() {
            sprite_queue.draw_sprite(&self.cursor_sprite);
        }

        self.category_menu.draw(game_io, &mut sprite_queue);
        self.location_menu.draw(game_io, &mut sprite_queue);

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn case_insensitive_contains(a: &str, b: &str) -> bool {
    let mut i = 0;

    let mut char_iter = a.chars();

    while let Some(c) = char_iter.clone().next() {
        if a.len() - i < b.len() {
            break;
        }

        if char_iter
            .clone()
            .zip(b.chars())
            .all(|(char_a, char_b)| char_a.eq_ignore_ascii_case(&char_b))
        {
            return true;
        }

        char_iter.next();

        i += c.len();
    }

    false
}

#[cfg(test)]
mod test {

    #[test]
    fn case_insensitive_contains() {
        let contains = super::case_insensitive_contains;

        assert!(contains("A", ""), "empty query");
        assert!(contains("A", "a"), "same length, diff capitalization");
        assert!(contains("A", "A"), "same length, same capitalization");
        assert!(!contains("A", "b"), "same length, bad query");
        assert!(contains("dcba", "a"), "short query, end of string");
        assert!(!contains("dcba", "x"), "short query, not in string");
        assert!(contains("dcAb", "ab"), "long query, end of string");
        assert!(!contains("a", "aa"), "longer query");
        assert!(!contains("A", "bb"), "longer query, not in string");
        assert!(!contains("JudgeMn", "erase"), "non-ascii, not in string");
        assert!(contains("JudgeMnEraseMn", "erase"), "non-ascii");
    }
}
