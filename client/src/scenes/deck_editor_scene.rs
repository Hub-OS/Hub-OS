use crate::bindable::CardClass;
use crate::bindable::Element;
use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::{Card, Deck, GlobalSave};
use framework::prelude::*;
use std::collections::HashMap;

const NAMESPACE: PackageNamespace = PackageNamespace::Local;
const MOVE_BULK_DELAY: FrameTime = 60;

#[derive(Clone, Copy, PartialEq, Eq)]
enum EditorMode {
    Default,
    SelectRegular,
}

enum Event {
    Leave(bool),
    Search(String),
    SwitchMode(EditorMode),
}

#[repr(u8)]
#[derive(Clone, Copy, PartialEq)]
enum Sorting {
    Id,
    Alphabetical,
    Code,
    Damage,
    Element,
    Number,
}

impl Sorting {
    fn code_sort_value(code: &str) -> u8 {
        let first_char = code.chars().next().unwrap_or_default();

        if first_char.is_alphabetic() {
            let mut buffer = [0u8; 4];
            let char = code.chars().next().unwrap_or_default();
            char.encode_utf8(&mut buffer);
            return buffer[0] - 65;
        }

        code.bytes().next().unwrap_or(u8::MAX)
    }

    fn generate_sort_keys<'a>(
        globals: &'a Globals,
        item: &CardListItem,
    ) -> (Option<&'a CardPackage>, (CardClass, PackageId, u8)) {
        let (package, card_class) = globals
            .card_packages
            .package(NAMESPACE, &item.card.package_id)
            .map(|package| (Some(package), package.card_properties.card_class))
            .unwrap_or_default();

        (
            package,
            (
                card_class,
                item.card.package_id.clone(),
                Self::code_sort_value(&item.card.code),
            ),
        )
    }

    fn sort_card_items<F, K>(card_slots: &mut [Option<CardListItem>], key_function: F)
    where
        F: FnMut(&CardListItem) -> K + Copy,
        K: std::cmp::Ord,
    {
        card_slots.sort_by_cached_key(move |slot| slot.as_ref().map(key_function));
    }

    fn sort_items(self, globals: &Globals, card_slots: &mut [Option<CardListItem>]) {
        match self {
            Sorting::Id => Self::sort_card_items(card_slots, |item: &CardListItem| {
                let (_, keys) = Self::generate_sort_keys(globals, item);

                keys
            }),
            Sorting::Alphabetical => Self::sort_card_items(card_slots, |item: &CardListItem| {
                let (package, keys) = Self::generate_sort_keys(globals, item);
                let short_name = package
                    .map(|package| package.card_properties.short_name.clone())
                    .unwrap_or_default();

                (short_name, keys)
            }),
            Sorting::Code => Self::sort_card_items(card_slots, |item: &CardListItem| {
                let (_, (class, package_id, code)) = Self::generate_sort_keys(globals, item);

                (code, class, package_id)
            }),
            Sorting::Damage => Self::sort_card_items(card_slots, |item: &CardListItem| {
                let (package, keys) = Self::generate_sort_keys(globals, item);
                let damage = package
                    .map(|package| package.card_properties.damage)
                    .unwrap_or_default();

                (damage, keys)
            }),
            Sorting::Element => Self::sort_card_items(card_slots, |item: &CardListItem| {
                let (package, keys) = Self::generate_sort_keys(globals, item);
                let element = package
                    .map(|package| package.card_properties.element)
                    .unwrap_or_default();

                (element, keys)
            }),
            Sorting::Number => Self::sort_card_items(card_slots, |item: &CardListItem| {
                let (_, keys) = Self::generate_sort_keys(globals, item);

                (-item.count, keys)
            }),
        }
    }
}

#[repr(u8)]
#[derive(Clone, Copy, PartialEq)]
enum SearchOption {
    Search,
    Namespace,
    Properties,
    Sort,
}

pub struct DeckEditorScene {
    deck_index: usize,
    deck_restrictions: DeckRestrictions,
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
    frame: SubSceneFrame,
    ui_input_tracker: UiInputTracker,
    confirm_held_time: FrameTime,
    confirm_interrupted: bool,
    select_card_on_release: bool,
    scene_time: FrameTime,
    page_tracker: PageTracker,
    pack_menu: ContextMenu<SearchOption>,
    namespace_menu: CommandPalette<usize>,
    properties_menu: CardPropertiesMenu,
    sort_menu: ContextMenu<Sorting>,
    name_filter: String,
    ns_filter: usize,
    filtered: bool,
    last_sort: Option<Sorting>,
    mode: EditorMode,
    deck_dock: Dock,
    pack_dock: Dock,
    deck_total_sprite: Sprite,
    deck_total_text: Text,
    textbox: Textbox,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    next_scene: NextScene,
}

impl DeckEditorScene {
    pub fn new(game_io: &GameIO, deck_index: usize) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();

        // limits
        let global_save = &globals.global_save;
        let restrictions = &globals.restrictions;
        let mut deck_restrictions = restrictions.base_deck_restrictions();

        if let Some(player_package) = global_save.player_package(game_io) {
            let script_enabled = restrictions.validate_player(game_io, player_package);

            deck_restrictions.apply_augments(
                script_enabled.then_some(player_package),
                global_save.valid_augments(game_io),
            );
        }

        // camera
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // ui
        let assets = &globals.assets;
        let ui_sprite = assets.new_sprite(game_io, ResourcePaths::DECK_UI);
        let mut ui_animator = Animator::load_new(assets, ResourcePaths::DECK_UI_ANIMATION);

        // total sprite
        let mut deck_total_sprite = ui_sprite.clone();
        ui_animator.set_state("TOTAL_FRAME");
        ui_animator.apply(&mut deck_total_sprite);

        // total text
        let mut deck_total_text = Text::new_monospace(game_io, FontName::Code);
        let deck_total_position = ui_animator.point_or_zero("TEXT_START") - ui_animator.origin();

        (deck_total_text.style.bounds).set_position(deck_total_position);
        deck_total_text.style.shadow_color = TEXT_DARK_SHADOW_COLOR;

        // deck_dock
        let deck = &globals.global_save.decks[deck_index];
        let mut deck_dock = Dock::new(
            game_io,
            CardListItem::vec_from_deck(&deck_restrictions, deck),
            ui_sprite.clone(),
            ui_animator.clone().with_state("DECK_DOCK"),
        );
        deck_dock.validate(game_io, &deck_restrictions);

        // pack_dock
        let mut pack_slots = CardListItem::pack_vec_from_packages(game_io, deck);
        Sorting::Id.sort_items(globals, &mut pack_slots);

        let pack_dock = Dock::new(
            game_io,
            pack_slots,
            ui_sprite.clone(),
            ui_animator.with_state("PACK_DOCK"),
        );

        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            deck_index,
            deck_restrictions,
            camera,
            background: Background::new_sub_scene(game_io),
            scene_title: SceneTitle::new(game_io, "deck-editor-scene-title"),
            frame: SubSceneFrame::new(game_io).with_top_bar(true),
            ui_input_tracker: UiInputTracker::new(),
            confirm_held_time: 0,
            confirm_interrupted: false,
            select_card_on_release: false,
            scene_time: 0,
            page_tracker: PageTracker::new(game_io, 2)
                .with_page_arrow_offset(0, pack_dock.page_arrow_offset),
            pack_menu: ContextMenu::new_translated(
                game_io,
                "deck-editor-pack-menu-label",
                Vec2::ZERO,
            )
            .with_translated_options(
                game_io,
                &[
                    ("deck-editor-option-search", SearchOption::Search),
                    ("deck-editor-option-namespace", SearchOption::Namespace),
                    ("deck-editor-option-properties", SearchOption::Properties),
                    ("deck-editor-option-sort", SearchOption::Sort),
                ],
            ),
            namespace_menu: CommandPalette::new(game_io, "deck-editor-namespace-menu-label")
                .with_options(Self::resolve_namespace_options(game_io)),
            properties_menu: CardPropertiesMenu::new(game_io),
            sort_menu: ContextMenu::new_translated(
                game_io,
                "deck-editor-sort-menu-label",
                Vec2::ZERO,
            )
            .with_translated_options(
                game_io,
                &[
                    ("deck-editor-sort-id", Sorting::Id),
                    ("deck-editor-sort-alphabetical", Sorting::Alphabetical),
                    ("deck-editor-sort-code", Sorting::Code),
                    ("deck-editor-sort-damage", Sorting::Damage),
                    ("deck-editor-sort-element", Sorting::Element),
                    ("deck-editor-sort-count", Sorting::Number),
                ],
            ),
            name_filter: Default::default(),
            ns_filter: Default::default(),
            filtered: false,
            last_sort: None,
            mode: EditorMode::Default,
            deck_dock,
            pack_dock,
            deck_total_sprite,
            deck_total_text,
            textbox: Textbox::new_navigation(game_io),
            event_sender,
            event_receiver,
            next_scene: NextScene::None,
        }
    }

    fn resolve_namespace_options(game_io: &GameIO) -> Vec<(String, usize)> {
        let globals = game_io.resource::<Globals>().unwrap();

        let namespaces = PackageId::prefixes_for_ids(globals.card_packages.package_ids(NAMESPACE));
        let mut namespaces: Vec<_> = namespaces.into_iter().collect();
        namespaces.sort();
        namespaces.insert(0, "");

        namespaces
            .into_iter()
            .enumerate()
            .map(|(i, s)| (s.to_string() + "*", i))
            .collect()
    }

    fn create_deck(&self, game_io: &GameIO) -> Deck {
        let global_save = &game_io.resource::<Globals>().unwrap().global_save;
        let old_deck = &global_save.decks[self.deck_index];

        let cards = self
            .deck_dock
            .card_slots
            .iter()
            .flatten()
            .map(|item| item.card.clone())
            .collect();

        Deck {
            name: old_deck.name.clone(),
            cards,
            regular_index: self.resolve_regular_index(),
            uuid: old_deck.uuid,
            update_time: old_deck.update_time,
        }
    }

    fn save_deck(&self, game_io: &mut GameIO, mut deck: Deck) {
        let global_save = &mut game_io.resource_mut::<Globals>().unwrap().global_save;
        deck.update_time = GlobalSave::current_time();
        global_save.decks[self.deck_index] = deck;
        global_save.save();
    }

    fn equip_deck(&self, game_io: &mut GameIO) {
        let global_save = &mut game_io.resource_mut::<Globals>().unwrap().global_save;
        global_save.selected_deck = self.deck_index;
        global_save.selected_deck_time = GlobalSave::current_time();
        global_save.save();
    }

    fn leave(&mut self, game_io: &mut GameIO) {
        let transition = crate::transitions::new_sub_scene_pop(game_io);
        self.next_scene = NextScene::new_pop().with_transition(transition);
    }

    fn resolve_regular_index(&self) -> Option<usize> {
        self.deck_dock
            .card_slots
            .iter()
            .flatten()
            .enumerate()
            .find(|(_, item)| item.is_regular)
            .map(|(i, _)| i)
    }
}

macro_rules! resolve_active_and_inactive_dock {
    ($scene: ident) => {
        match $scene.page_tracker.active_page() {
            0 => (&mut $scene.deck_dock, &mut $scene.pack_dock),
            1 => (&mut $scene.pack_dock, &mut $scene.deck_dock),
            _ => unreachable!(),
        }
    };
}

impl Scene for DeckEditorScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.scene_time += 1;

        self.page_tracker.update();

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        self.textbox.update(game_io);

        // input
        if game_io.is_in_transition() {
            return;
        }

        handle_events(self, game_io);

        if self.textbox.is_open() {
            return;
        }

        handle_input(self, game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw title
        self.frame.draw(&mut sprite_queue);
        self.scene_title.draw(game_io, &mut sprite_queue);

        // draw docks
        for (page, offset) in self.page_tracker.visible_pages() {
            let dock = match page {
                0 => &mut self.deck_dock,
                1 => &mut self.pack_dock,
                _ => unreachable!(),
            };

            dock.draw(game_io, &mut sprite_queue, self.mode, offset);

            let menu_open = self.textbox.is_open()
                || self.pack_menu.is_open()
                || self.sort_menu.is_open()
                || self.namespace_menu.is_open()
                || self.properties_menu.is_open();

            if !menu_open {
                dock.draw_cursor(&mut sprite_queue, offset);
            }
        }

        // draw page_arrows
        self.page_tracker.draw_page_arrows(&mut sprite_queue);

        // draw context menu
        if self.pack_menu.is_open() {
            let (active_dock, _) = resolve_active_and_inactive_dock!(self);

            let position = active_dock.context_menu_position;
            self.pack_menu.recalculate_layout(game_io);
            self.pack_menu.set_top_center(position);
            self.pack_menu.draw(game_io, &mut sprite_queue);
        } else if self.sort_menu.is_open() {
            let (active_dock, _) = resolve_active_and_inactive_dock!(self);

            let position = active_dock.context_menu_position;
            self.sort_menu.recalculate_layout(game_io);
            self.sort_menu.set_top_center(position);
            self.sort_menu.draw(game_io, &mut sprite_queue);
        } else if self.namespace_menu.is_open() {
            self.namespace_menu.draw(game_io, &mut sprite_queue);
        } else if self.properties_menu.is_open() {
            self.properties_menu.draw(game_io, &mut sprite_queue);
        }

        // draw deck total frame
        let offset = Vec2::new(self.page_tracker.page_offset(0), 0.0);
        let original_total_position = self.deck_total_sprite.position();
        let adjusted_total_position = original_total_position + offset;

        self.deck_total_sprite.set_position(adjusted_total_position);
        sprite_queue.draw_sprite(&self.deck_total_sprite);
        self.deck_total_sprite.set_position(original_total_position);

        // draw deck total
        let card_count = self.deck_dock.card_count;

        let original_total_pos = self.deck_total_text.style.bounds.position();
        self.deck_total_text.style.bounds += offset;

        self.deck_total_text.style.color = if card_count == self.deck_restrictions.required_total {
            Color::from((173, 255, 189))
        } else {
            Color::from((255, 181, 74))
        };

        self.deck_total_text.text =
            format!("{card_count:>2}/{}", self.deck_restrictions.required_total);
        self.deck_total_text.draw(game_io, &mut sprite_queue);

        (self.deck_total_text.style.bounds).set_position(original_total_pos);

        // draw textbox over everything else
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn handle_events(scene: &mut DeckEditorScene, game_io: &mut GameIO) {
    let Ok(event) = scene.event_receiver.try_recv() else {
        return;
    };

    match event {
        Event::Leave(equip) => {
            if equip {
                scene.equip_deck(game_io);
            }

            scene.leave(game_io);
        }
        Event::SwitchMode(mode) => {
            match mode {
                EditorMode::Default => {}
                EditorMode::SelectRegular => {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let message = globals.translate("deck-editor-regular-card-mode-start");
                    let interface = TextboxMessage::new(message);
                    scene.textbox.push_interface(interface);
                }
            }

            scene.mode = mode;
        }
        Event::Search(search_text) => {
            scene.name_filter = search_text.to_lowercase();
            apply_filters(scene, game_io);
            scene.pack_dock.update_preview(game_io);
        }
    }
}

/// only runs on chips in the pack
fn apply_filters(scene: &mut DeckEditorScene, game_io: &mut GameIO) {
    let globals = game_io.resource::<Globals>().unwrap();
    let name_filter = &scene.name_filter;

    if scene.filtered {
        // previously filtered? reset pack
        let deck = scene.create_deck(game_io);

        let dock = &mut scene.pack_dock;
        dock.card_slots = CardListItem::pack_vec_from_packages(game_io, &deck);

        Sorting::Id.sort_items(globals, &mut dock.card_slots);
    }

    let dock = &mut scene.pack_dock;

    if name_filter.is_empty()
        && scene.ns_filter == 0
        && scene.properties_menu.applied_filters().next().is_none()
    {
        // no filters
        dock.scroll_tracker.set_selected_index(0);
        dock.scroll_tracker.set_total_items(dock.card_slots.len());
        return;
    }

    scene.filtered = true;

    let packages = &globals.card_packages;

    // apply namespace filter
    if scene.ns_filter > 0 {
        let Some(label) = scene.namespace_menu.get_label(scene.ns_filter) else {
            log::error!("Namespace filter is out of bounds?");
            return;
        };

        // strip *
        let ns_filter = &label[..label.len() - 1];

        dock.card_slots.retain(|slot| {
            slot.as_ref()
                .is_some_and(|item| item.card.package_id.as_str().starts_with(ns_filter))
        });
    }

    // apply name filter
    if !name_filter.is_empty() {
        dock.card_slots.retain(|slot| {
            slot.as_ref()
                .and_then(|item| packages.package(NAMESPACE, &item.card.package_id))
                .is_some_and(|package| package.search_name.contains(name_filter))
        });
    }

    // apply properties filter
    for filter in scene.properties_menu.applied_filters() {
        dock.card_slots.retain(|slot| {
            slot.as_ref()
                .and_then(|item| packages.package(NAMESPACE, &item.card.package_id))
                .is_some_and(|package| {
                    let properties = &package.card_properties;

                    match filter {
                        CardPropertyFilter::Element(Element::None) => {
                            properties.element == Element::None
                        }
                        CardPropertyFilter::Element(element) => {
                            properties.element == *element
                                || properties.secondary_element == *element
                        }
                        CardPropertyFilter::HitFlag { flag_name, id, .. } => {
                            properties.hit_flags.iter().any(|f| f == &**flag_name)
                                || package.package_info.requirements.iter().any(
                                    |(category, dep_id)| {
                                        if *category != PackageCategory::Status {
                                            return false;
                                        }

                                        id == dep_id
                                    },
                                )
                        }
                        CardPropertyFilter::Chargeable => properties.can_charge,
                        CardPropertyFilter::Boostable => properties.can_boost,
                        CardPropertyFilter::Recovers => properties.recover != 0,
                        CardPropertyFilter::TimeFreeze => properties.time_freeze,
                        CardPropertyFilter::Conceal => properties.conceal,
                    }
                })
        });
    }

    dock.scroll_tracker.set_selected_index(0);
    dock.scroll_tracker.set_total_items(dock.card_slots.len());
}

fn handle_input(scene: &mut DeckEditorScene, game_io: &mut GameIO) {
    scene.ui_input_tracker.update(game_io);

    if handle_any_context_input(scene, game_io) {
        scene.confirm_held_time = 0;
        scene.confirm_interrupted = true;
        return;
    }

    // dock scrolling
    let (active_dock, _) = resolve_active_and_inactive_dock!(scene);

    let scroll_tracker = &mut active_dock.scroll_tracker;
    let original_index = scroll_tracker.selected_index();

    scroll_tracker.handle_vertical_input(&scene.ui_input_tracker);

    if original_index != scroll_tracker.selected_index() {
        active_dock.update_preview(game_io);
        scene.confirm_interrupted = true;

        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_move);
    }

    // selecting dock
    let input_util = InputUtil::new(game_io);

    let previous_page = scene.page_tracker.active_page();

    if scene.mode == EditorMode::Default {
        scene.page_tracker.handle_input(game_io);
    }

    if previous_page != scene.page_tracker.active_page() {
        scene.last_sort = None;
    }

    // see if we should do something other than default handling based on mode
    match scene.mode {
        EditorMode::Default => {
            handle_default_mode_input(scene, game_io);
        }
        EditorMode::SelectRegular => {
            handle_select_regular_input(scene, game_io);
        }
    }

    // cancelling
    if input_util.was_just_pressed(Input::Cancel) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_cancel);

        if scene.mode == EditorMode::SelectRegular {
            // revert to default mode
            scene.mode = EditorMode::Default;
            return;
        }

        // default handling
        let cancel_handled = scene.deck_dock.scroll_tracker.forget_index().is_some()
            || scene.pack_dock.scroll_tracker.forget_index().is_some();

        // closing
        if !cancel_handled {
            let old_deck = &globals.global_save.decks[scene.deck_index];
            let deck = scene.create_deck(game_io);
            let is_equipped = globals.global_save.selected_deck == scene.deck_index;
            let deck_updated = *old_deck != deck;

            if is_equipped || !deck_updated {
                // didn't modify the deck, leave without changing the equipped deck
                scene.leave(game_io);
            } else {
                let event_sender = scene.event_sender.clone();
                let callback = move |response| {
                    event_sender.send(Event::Leave(response)).unwrap();
                };

                let question = globals.translate_with_args(
                    "deck-editor-equip-deck-question",
                    vec![("name", (&old_deck.name).into())],
                );
                let textbox_interface = TextboxQuestion::new(game_io, question, callback);

                scene.textbox.push_interface(textbox_interface);
                scene.textbox.open();
            }

            if deck_updated {
                scene.save_deck(game_io, deck);
            }

            return;
        }
    }

    // context menu
    if input_util.was_just_pressed(Input::Option) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_select);

        if scene.page_tracker.active_page() == 0 {
            scene.sort_menu.open();
        } else {
            scene.pack_menu.open();
        }
    }

    // flip card previews
    if input_util.was_just_pressed(Input::Special) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_select);

        scene.deck_dock.card_preview.toggle_flipped();
        scene.pack_dock.card_preview.toggle_flipped();
    }

    // handle selecting regular card
    if scene.page_tracker.active_page() == 0 && input_util.was_released(Input::Option2) {
        let event_sender = scene.event_sender.clone();

        let globals = game_io.resource::<Globals>().unwrap();
        let message = globals.translate("deck-editor-regular-card-mode-question");
        let interface = TextboxQuestion::new(game_io, message, move |yes| {
            if yes {
                event_sender
                    .send(Event::SwitchMode(EditorMode::SelectRegular))
                    .unwrap();
            }
        });

        scene.textbox.push_interface(interface);
        scene.textbox.open();
    }
}

fn handle_default_mode_input(scene: &mut DeckEditorScene, game_io: &GameIO) {
    let input_util = InputUtil::new(game_io);

    if input_util.was_just_pressed(Input::Confirm) {
        scene.confirm_interrupted = false;
        scene.confirm_held_time = 1;

        let (active_dock, _) = resolve_active_and_inactive_dock!(scene);
        let scroll_tracker = &active_dock.scroll_tracker;

        if scroll_tracker.remembered_index() != Some(scroll_tracker.selected_index()) {
            select_card(scene, game_io);
            scene.select_card_on_release = false;
        } else {
            scene.select_card_on_release = scroll_tracker.remembered_index().is_some();
        }
    }

    if scene.confirm_interrupted {
        return;
    }

    if input_util.is_down(Input::Confirm) {
        scene.confirm_held_time += 1;

        let (active_dock, _) = resolve_active_and_inactive_dock!(scene);
        let scroll_tracker = &active_dock.scroll_tracker;

        if scene.confirm_held_time == MOVE_BULK_DELAY
            && scroll_tracker.remembered_index() == Some(scroll_tracker.selected_index())
        {
            move_in_bulk(scene, game_io);
        }
    } else if scene.confirm_held_time > 0 {
        if scene.select_card_on_release && scene.confirm_held_time < MOVE_BULK_DELAY {
            select_card(scene, game_io);
        }

        scene.confirm_held_time = 0;
    }
}

fn handle_select_regular_input(scene: &mut DeckEditorScene, game_io: &GameIO) {
    let input_util = InputUtil::new(game_io);

    if input_util.was_just_pressed(Input::Confirm) {
        select_regular_card(scene, game_io);
    }
}

fn handle_any_context_input(scene: &mut DeckEditorScene, game_io: &mut GameIO) -> bool {
    if scene.pack_menu.is_open() {
        handle_options_menu_input(scene, game_io);
        return true;
    }

    if scene.namespace_menu.is_open() {
        handle_namespace_menu_input(scene, game_io);
        return true;
    }

    if scene.properties_menu.is_open() {
        handle_properties_menu_input(scene, game_io);
        return true;
    }

    if scene.sort_menu.is_open() {
        handle_sort_menu_input(scene, game_io);
        return true;
    }

    false
}

fn handle_options_menu_input(scene: &mut DeckEditorScene, game_io: &mut GameIO) {
    let input_util = InputUtil::new(game_io);

    // closing menu
    if input_util.was_just_pressed(Input::Option) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_cancel);

        scene.pack_menu.close();
        return;
    }

    let Some(selected_option) = scene.pack_menu.update(game_io, &scene.ui_input_tracker) else {
        return;
    };

    scene.pack_menu.close();

    match selected_option {
        SearchOption::Search => {
            let event_sender = scene.event_sender.clone();

            let interface = TextboxPrompt::new(move |s| {
                let _ = event_sender.send(Event::Search(s));
            })
            .with_character_limit(8);

            scene.textbox.push_interface(interface);
            scene.textbox.open();
        }
        SearchOption::Namespace => {
            scene.namespace_menu.open();
        }
        SearchOption::Properties => {
            scene.properties_menu.open(game_io);
        }
        SearchOption::Sort => {
            scene.sort_menu.open();
        }
    }
}

fn handle_namespace_menu_input(scene: &mut DeckEditorScene, game_io: &mut GameIO) {
    let input_util = InputUtil::new(game_io);

    // closing menu
    if input_util.was_just_pressed(Input::Option) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_cancel);

        scene.namespace_menu.close();
        return;
    }

    let Some(i) = scene
        .namespace_menu
        .update(game_io, &scene.ui_input_tracker)
    else {
        return;
    };

    scene.namespace_menu.close();

    scene.ns_filter = i;
    scene.name_filter = Default::default();
    apply_filters(scene, game_io);
    scene.pack_dock.update_preview(game_io);
}

fn handle_properties_menu_input(scene: &mut DeckEditorScene, game_io: &mut GameIO) {
    let input_util = InputUtil::new(game_io);

    // closing menu
    if input_util.was_just_pressed(Input::Option) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_cancel);

        scene.properties_menu.close();
    } else {
        scene
            .properties_menu
            .update(game_io, &scene.ui_input_tracker);
    }

    if !scene.properties_menu.is_open() {
        apply_filters(scene, game_io);
        scene.pack_dock.update_preview(game_io);
    }
}

fn handle_sort_menu_input(scene: &mut DeckEditorScene, game_io: &mut GameIO) {
    let input_util = InputUtil::new(game_io);

    // closing menu
    if input_util.was_just_pressed(Input::Option) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_cancel);

        scene.sort_menu.close();
        return;
    }

    let Some(selected_option) = scene.sort_menu.update(game_io, &scene.ui_input_tracker) else {
        return;
    };

    let (dock, _) = resolve_active_and_inactive_dock!(scene);
    let card_slots = &mut dock.card_slots;

    // silly order preservation fix for the later reverse call
    if scene.last_sort == Some(selected_option) {
        card_slots.reverse();
    }

    // save selections to restore after sorting
    let remembered_value = dock
        .scroll_tracker
        .remembered_index()
        .map(|i| card_slots.get(i).unwrap_or(&None).clone());

    // sort
    let globals = game_io.resource::<Globals>().unwrap();
    selected_option.sort_items(globals, card_slots);

    if scene.last_sort.take() == Some(selected_option) {
        card_slots.reverse();
    } else {
        scene.last_sort = Some(selected_option);
    }

    // restore selections
    if let Some(value) = remembered_value {
        let new_index = card_slots
            .iter()
            .position(|slot| *slot == value)
            .unwrap_or_default();

        dock.scroll_tracker.set_remembered_index(new_index)
    }

    // blanks should always be at the bottom
    card_slots.sort_by_key(|item| !item.is_some());

    dock.update_preview(game_io);
}

fn move_in_bulk(scene: &mut DeckEditorScene, game_io: &GameIO) {
    scene.deck_dock.scroll_tracker.forget_index();
    scene.pack_dock.scroll_tracker.forget_index();

    let success = if scene.page_tracker.active_page() == 0 {
        move_bulk_to_pack(scene, game_io).is_ok()
    } else {
        move_bulk_to_deck(scene, game_io).is_ok()
    };

    let globals = game_io.resource::<Globals>().unwrap();

    if success {
        globals.audio.play_sound(&globals.sfx.page_turn);
        scene.deck_dock.validate(game_io, &scene.deck_restrictions);
        scene.pack_dock.update_preview(game_io);
        scene.deck_dock.update_preview(game_io);
    } else {
        globals.audio.play_sound(&globals.sfx.cursor_error);
    }
}

fn move_bulk_to_pack(scene: &mut DeckEditorScene, game_io: &GameIO) -> Result<(), ()> {
    let index = scene.deck_dock.scroll_tracker.selected_index();

    let Some(Some(card_item)) = scene.deck_dock.card_slots.get(index) else {
        return Err(());
    };

    let id = card_item.card.package_id.clone();
    let code = card_item.card.code.clone();

    let pack_index = transfer_to_pack(scene, game_io, index);

    // count and remove duplicates
    let mut duplicate_count = 0;

    for slot in scene.deck_dock.card_slots.iter_mut() {
        if slot
            .as_ref()
            .is_some_and(|item| item.card.package_id == id && item.card.code == code)
        {
            *slot = None;
            duplicate_count += 1;
        }
    }

    if let Some(index) = pack_index
        && let Some(slot) = scene.pack_dock.card_slots[index].as_mut()
    {
        slot.count += duplicate_count;
    }

    transfer_to_pack_cleanup(scene, pack_index);
    Ok(())
}

fn move_bulk_to_deck(scene: &mut DeckEditorScene, game_io: &GameIO) -> Result<(), ()> {
    let index = scene.pack_dock.scroll_tracker.selected_index();

    let Some(Some(card_item)) = scene.pack_dock.card_slots.get(index) else {
        return Err(());
    };

    let count = card_item.count;

    transfer_to_deck(scene, game_io, index)?;

    // try to add duplicates
    for _ in 1..count {
        if transfer_to_deck(scene, game_io, index).is_err() {
            break;
        }
    }

    Ok(())
}

fn select_card(scene: &mut DeckEditorScene, game_io: &GameIO) {
    // default handling, moving cards around
    let (active_dock, inactive_dock) = resolve_active_and_inactive_dock!(scene);
    let mut success = true;

    if let Some(index) = active_dock.scroll_tracker.forget_index() {
        success = dock_internal_swap(scene, game_io, index).is_ok();
    } else if let Some(inactive_index) = inactive_dock.scroll_tracker.forget_index() {
        let active_index = active_dock.scroll_tracker.selected_index();
        success = inter_dock_swap(scene, game_io, inactive_index, active_index).is_ok();
    } else if active_dock.scroll_tracker.total_items() > 0 {
        active_dock.scroll_tracker.remember_index();
    } else {
        success = false;
    }

    let globals = game_io.resource::<Globals>().unwrap();

    if success {
        globals.audio.play_sound(&globals.sfx.cursor_select);
    } else {
        globals.audio.play_sound(&globals.sfx.cursor_error);
    }
}

fn select_regular_card(scene: &mut DeckEditorScene, game_io: &GameIO) {
    let globals = game_io.resource::<Globals>().unwrap();

    let deck_dock = &mut scene.deck_dock;
    let slots = &mut deck_dock.card_slots;
    let index = deck_dock.scroll_tracker.selected_index();

    let Some(item) = &mut slots[index] else {
        globals.audio.play_sound(&globals.sfx.cursor_error);
        return;
    };

    let package_id = &item.card.package_id;

    if !item.is_regular {
        let regular_allowed = item.valid && {
            let card_packages = &globals.card_packages;
            let package = card_packages.package(NAMESPACE, package_id);
            package.is_some_and(|package| package.regular_allowed)
        };

        if !regular_allowed {
            globals.audio.play_sound(&globals.sfx.cursor_error);
            return;
        }
    }

    item.is_regular = !item.is_regular;

    if item.is_regular {
        // unmark other slots
        for (i, slot) in slots.iter_mut().enumerate() {
            let Some(item) = slot else {
                continue;
            };

            if index != i {
                item.is_regular = false;
            }
        }

        let message = globals.translate("deck-editor-regular-card-select");
        let interface = TextboxMessage::new(message);
        scene.textbox.push_interface(interface);
        scene.textbox.open();

        globals.audio.play_sound(&globals.sfx.card_select_confirm);
    } else {
        let message = globals.translate("deck-editor-regular-card-deselect");
        let interface = TextboxMessage::new(message);
        scene.textbox.push_interface(interface);
        scene.textbox.open();

        globals.audio.play_sound(&globals.sfx.cursor_cancel);
    }

    scene.mode = EditorMode::Default;
}

fn dock_internal_swap(
    scene: &mut DeckEditorScene,
    game_io: &GameIO,
    index: usize,
) -> Result<(), ()> {
    if scene.page_tracker.active_page() == 0 {
        let selected_index = scene.deck_dock.scroll_tracker.selected_index();

        if index == selected_index {
            let pack_index = transfer_to_pack(scene, game_io, index);
            transfer_to_pack_cleanup(scene, pack_index);
        } else {
            scene.deck_dock.card_slots.swap(selected_index, index);
        }
    } else {
        let selected_index = scene.pack_dock.scroll_tracker.selected_index();

        if index == selected_index {
            transfer_to_deck(scene, game_io, index)?;
        } else {
            scene.pack_dock.card_slots.swap(selected_index, index);
        }
    }

    Ok(())
}

fn inter_dock_swap(
    scene: &mut DeckEditorScene,
    game_io: &GameIO,
    inactive_index: usize,
    active_index: usize,
) -> Result<(), ()> {
    let (deck_index, pack_index);

    if scene.page_tracker.active_page() == 0 {
        deck_index = active_index;
        pack_index = inactive_index;
    } else {
        deck_index = inactive_index;
        pack_index = active_index;
    }

    let pack_slots = &scene.pack_dock.card_slots;
    let empty_pack = pack_slots.is_empty();
    let pack_item = pack_slots.get(pack_index).and_then(|o| o.as_ref());
    let pack_card_count = pack_item.map(|item| item.count).unwrap_or_default();

    // store the index of the transferred card in case we need to move it back
    let mut stored_index = transfer_to_pack(scene, game_io, deck_index);

    let index = if empty_pack {
        // don't transfer the chip we moved to the pack back to the deck
        0
    } else if let Ok(index) = transfer_to_deck(scene, game_io, pack_index) {
        index
    } else {
        if transfer_to_pack_cleanup(scene, stored_index) {
            // converted negative cards in pack to 0
            return Ok(());
        }

        if let Some(stored_index) = stored_index {
            // move it back
            let index = transfer_to_deck(scene, game_io, stored_index)?;
            scene.deck_dock.card_slots.swap(index, deck_index);
        }

        return Err(());
    };

    if let Some(stored_index) = &mut stored_index
        && pack_card_count == 1
        && *stored_index > pack_index
    {
        let merged_item = scene.pack_dock.card_slots[*stored_index - 1].as_ref();
        let merged_card_count = merged_item.unwrap().count;

        if merged_card_count == 1 {
            // move the card transferred to the pack into the pack slot
            scene.pack_dock.card_slots.insert(pack_index, None);
            scene.pack_dock.card_slots.swap(*stored_index, pack_index);
            scene.pack_dock.card_slots.pop();
            *stored_index = pack_index;
        } else {
            *stored_index -= 1;
        }
    }

    // move the card transferred to the deck to the correct slot
    // otherwise it's moved to the first empty slot and not the one we're selecting
    scene.deck_dock.card_slots.swap(index, deck_index);

    transfer_to_pack_cleanup(scene, stored_index);
    scene.deck_dock.validate(game_io, &scene.deck_restrictions);
    scene.deck_dock.update_preview(game_io);
    scene.pack_dock.update_preview(game_io);

    Ok(())
}

fn transfer_to_deck(
    scene: &mut DeckEditorScene,
    game_io: &GameIO,
    from_index: usize,
) -> Result<usize, ()> {
    let card_slot = scene.pack_dock.card_slots.get_mut(from_index).ok_or(())?;
    let card_item = card_slot.as_mut().ok_or(())?;

    // can't move negative count
    // negative counts exist when an invalid deck is viewed
    if card_item.count <= 0 {
        return Err(());
    }

    let globals = game_io.resource::<Globals>().unwrap();
    let card_manager = &globals.card_packages;
    let package = card_manager
        .package(NAMESPACE, &card_item.card.package_id)
        .ok_or(())?;
    let deck_dock = &mut scene.deck_dock;

    // maintain duplicate limit requirement
    let package_count = deck_dock
        .card_items()
        .filter(|item| item.card.package_id == card_item.card.package_id)
        .count();

    if package_count >= package.limit {
        return Err(());
    }

    // maintain ownership requirement
    let restrictions = &globals.restrictions;

    let card_count = deck_dock
        .card_items()
        .filter(|item| item.card == card_item.card)
        .count();

    if card_count > restrictions.card_count(game_io, &card_item.card) {
        return Err(());
    }

    match package.card_properties.card_class {
        CardClass::Mega => {
            let mega_count = deck_dock.count_class(card_manager, CardClass::Mega);

            if mega_count >= scene.deck_restrictions.mega_limit {
                return Err(());
            }
        }
        CardClass::Giga => {
            let giga_count = deck_dock.count_class(card_manager, CardClass::Giga);

            if giga_count >= scene.deck_restrictions.giga_limit {
                return Err(());
            }
        }
        CardClass::Dark => {
            let dark_count = deck_dock.count_class(card_manager, CardClass::Dark);

            if dark_count >= scene.deck_restrictions.dark_limit {
                return Err(());
            }
        }
        _ => {}
    };

    // search for an empty slot to insert the card into
    let deck_slots = &mut scene.deck_dock.card_slots;

    let empty_index = deck_slots
        .iter_mut()
        .position(|item| item.is_none())
        .ok_or(())?;

    deck_slots[empty_index] = Some(CardListItem {
        card: card_item.card.clone(),
        valid: true, // we'll validate this later
        count: 1,
        show_count: false,
        is_regular: false,
    });

    card_item.count -= 1;

    if card_item.count == 0 {
        scene.pack_dock.card_slots.remove(from_index);

        let pack_size = scene.pack_dock.card_slots.len();
        scene.pack_dock.scroll_tracker.set_total_items(pack_size);
    }

    scene.deck_dock.validate(game_io, &scene.deck_restrictions);
    scene.deck_dock.update_preview(game_io);
    scene.pack_dock.update_preview(game_io);

    Ok(empty_index)
}

fn transfer_to_pack(
    scene: &mut DeckEditorScene,
    game_io: &GameIO,
    from_index: usize,
) -> Option<usize> {
    let card = scene.deck_dock.card_slots.get_mut(from_index)?.take()?.card;

    scene.deck_dock.validate(game_io, &scene.deck_restrictions);
    scene.deck_dock.update_preview(game_io);

    let pack_slots = &mut scene.pack_dock.card_slots;

    let pack_index = pack_slots
        .iter_mut()
        .position(|item| item.as_ref().unwrap().card == card);

    let Some(pack_index) = pack_index else {
        pack_slots.push(Some(CardListItem {
            card,
            valid: true,
            count: 1,
            show_count: true,
            is_regular: false,
        }));

        scene
            .pack_dock
            .scroll_tracker
            .set_total_items(pack_slots.len());

        return Some(pack_slots.len() - 1);
    };

    let item = pack_slots[pack_index].as_mut().unwrap();

    item.count += 1;

    Some(pack_index)
}

fn transfer_to_pack_cleanup(scene: &mut DeckEditorScene, pack_index: Option<usize>) -> bool {
    let Some(pack_index) = pack_index else {
        return false;
    };

    let slot = &scene.pack_dock.card_slots[pack_index];

    if slot.as_ref().is_some_and(|item| item.count != 0) {
        return false;
    }

    // remove slot
    scene.pack_dock.card_slots.remove(pack_index);

    // update pack size
    let pack_size = scene.pack_dock.card_slots.len();
    scene.pack_dock.scroll_tracker.set_total_items(pack_size);

    true
}

struct Dock {
    card_slots: Vec<Option<CardListItem>>,
    card_count: usize,
    scroll_tracker: ScrollTracker,
    dock_sprite: Sprite,
    dock_animator: Animator,
    regular_sprite: Sprite,
    card_preview: FullCard,
    list_position: Vec2,
    context_menu_position: Vec2,
    page_arrow_offset: Vec2,
}

impl Dock {
    fn new(
        game_io: &GameIO,
        card_slots: Vec<Option<CardListItem>>,
        mut dock_sprite: Sprite,
        mut dock_animator: Animator,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // dock
        dock_animator.set_loop_mode(AnimatorLoopMode::Loop);
        dock_animator.apply(&mut dock_sprite);

        let dock_offset = -dock_sprite.origin();

        let list_point = dock_animator.point_or_zero("LIST");
        let list_position = dock_offset + list_point;

        let context_menu_point = dock_animator.point_or_zero("CONTEXT_MENU");
        let context_menu_position = dock_offset + context_menu_point;

        let page_arrow_point = dock_animator.point_or_zero("PAGE_ARROWS");
        let page_arrow_offset = dock_offset + page_arrow_point;

        // regular sprite
        let mut regular_sprite = assets.new_sprite(game_io, ResourcePaths::REGULAR_CARD);
        let mut regular_animator =
            Animator::load_new(assets, ResourcePaths::REGULAR_CARD_ANIMATION);

        regular_animator.set_state("DEFAULT");
        regular_animator.apply(&mut regular_sprite);

        // card sprite
        let card_offset = dock_animator.point_or_zero("CARD");
        let card_preview = FullCard::new(game_io, dock_offset + card_offset);

        // scroll tracker
        let mut scroll_tracker = ScrollTracker::new(game_io, 7);
        scroll_tracker.set_total_items(card_slots.len());

        let scroll_start = dock_offset + dock_animator.point_or_zero("SCROLL_START");
        let scroll_end = dock_offset + dock_animator.point_or_zero("SCROLL_END");

        scroll_tracker.define_scrollbar(scroll_start, scroll_end);

        // cursor
        let cursor_start = dock_offset + dock_animator.point_or_zero("CURSOR_START");
        scroll_tracker.define_cursor(cursor_start, 16.0);

        let mut dock = Self {
            card_slots,
            card_count: 0,
            scroll_tracker,
            dock_sprite,
            dock_animator,
            regular_sprite,
            card_preview,
            list_position,
            context_menu_position,
            page_arrow_offset,
        };

        dock.update_preview(game_io);

        dock
    }

    fn card_items(&self) -> impl Iterator<Item = &CardListItem> {
        self.card_slots.iter().flat_map(|item| item.as_ref())
    }

    fn count_class(
        &self,
        card_manager: &PackageManager<CardPackage>,
        card_class: CardClass,
    ) -> usize {
        self.card_items()
            .flat_map(|item| card_manager.package(NAMESPACE, &item.card.package_id))
            .filter(|package| package.card_properties.card_class == card_class)
            .count()
    }

    fn validate(&mut self, game_io: &GameIO, deck_restrictions: &DeckRestrictions) {
        let card_iterator = self.card_slots.iter().flatten().map(|item| &item.card);
        let deck_validation = deck_restrictions.validate_cards(game_io, NAMESPACE, card_iterator);
        let mut total_cards = 0;

        for card_item in self.card_slots.iter_mut().flatten() {
            card_item.valid = deck_validation.is_card_valid(&card_item.card);
            total_cards += 1;
        }

        self.card_count = total_cards;
    }

    fn update_preview(&mut self, game_io: &GameIO) {
        let selected_index = self.scroll_tracker.selected_index();

        let card = self
            .card_slots
            .get(selected_index)
            .and_then(|item| item.as_ref().map(|item| item.card.clone()));

        self.card_preview.set_card(game_io, card);
    }

    fn draw(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        mode: EditorMode,
        offset_x: f32,
    ) {
        let offset = Vec2::new(offset_x, 0.0);

        self.dock_animator.update();
        self.dock_animator.apply(&mut self.dock_sprite);
        self.dock_sprite.set_position(offset);
        sprite_queue.draw_sprite(&self.dock_sprite);

        // draw card preview using offset
        let card_position = self.card_preview.position();
        self.card_preview.set_position(card_position + offset);

        self.card_preview.draw(game_io, sprite_queue);

        self.card_preview.set_position(card_position);

        // draw list items
        for i in self.scroll_tracker.view_range() {
            let Some(card_item) = &self.card_slots[i] else {
                continue;
            };

            let relative_index = i - self.scroll_tracker.top_index();

            let mut position = self.list_position + offset;
            position.y += relative_index as f32 * self.scroll_tracker.cursor_multiplier();

            card_item.draw_list_item(game_io, sprite_queue, mode, position);

            if card_item.is_regular {
                self.regular_sprite.set_position(position);
                sprite_queue.draw_sprite(&self.regular_sprite);
            }
        }

        // draw scrollbar
        let (start, end) = self.scroll_tracker.scrollbar_definition();
        self.scroll_tracker
            .define_scrollbar(start + offset, end + offset);

        self.scroll_tracker.draw_scrollbar(sprite_queue);

        self.scroll_tracker.define_scrollbar(start, end);
    }

    fn draw_cursor(&mut self, sprite_queue: &mut SpriteColorQueue, offset: f32) {
        let (start, multiplier) = self.scroll_tracker.cursor_definition();
        self.scroll_tracker
            .define_cursor(start + Vec2::new(offset, 0.0), multiplier);

        self.scroll_tracker.draw_cursor(sprite_queue);

        self.scroll_tracker.define_cursor(start, multiplier);
    }
}

#[derive(Clone, PartialEq, Eq)]
struct CardListItem {
    card: Card,
    valid: bool,
    count: isize,
    show_count: bool,
    is_regular: bool,
}

impl CardListItem {
    pub fn vec_from_deck(
        deck_restrictions: &DeckRestrictions,
        deck: &Deck,
    ) -> Vec<Option<CardListItem>> {
        let mut card_items: Vec<_> = deck
            .cards
            .iter()
            .enumerate()
            .map(|(i, card)| {
                Some(CardListItem {
                    card: card.clone(),
                    valid: true,
                    count: 0,
                    show_count: false,
                    is_regular: deck.regular_index == Some(i),
                })
            })
            .collect();

        // ensure enough slots for the total requirement
        if card_items.len() < deck_restrictions.required_total {
            card_items.resize_with(deck_restrictions.required_total, || None);
        }

        card_items
    }

    pub fn pack_vec_from_packages(
        game_io: &GameIO,
        active_deck: &Deck,
    ) -> Vec<Option<CardListItem>> {
        let globals = game_io.resource::<Globals>().unwrap();
        let package_manager = &globals.card_packages;
        let restrictions = &globals.restrictions;

        // track card usage for resolving cards remaining in pack
        let mut use_counts = HashMap::new();

        for card in &active_deck.cards {
            use_counts
                .entry(card)
                .and_modify(|count| *count += 1)
                .or_insert(1);
        }

        let map_to_item_with_count = |(valid_package, card, use_limit)| {
            let use_count = use_counts.get(&card).cloned().unwrap_or_default();
            let count = use_limit as isize - use_count;

            CardListItem {
                card,
                valid: valid_package && count > 0,
                count,
                show_count: true,
                is_regular: false,
            }
        };

        if restrictions.card_ownership_enabled() {
            // use owned cards list + existing cards in folder for pack
            let mut package_validity = HashMap::new();

            let existing_iter = use_counts.keys().map(|card| (*card, 0));
            let owned_iter = restrictions.card_iter().filter(|(card, _)| {
                // filter for installed cards only
                package_manager
                    .package(NAMESPACE, &card.package_id)
                    .is_some()
            });

            let cards: HashMap<_, _> = existing_iter.chain(owned_iter).collect();

            cards
                .into_iter()
                .map(|(card, count)| {
                    let valid_package =
                        *package_validity.entry(&card.package_id).or_insert_with(|| {
                            let package_id = card.package_id.clone();
                            let triplet = (PackageCategory::Card, NAMESPACE, package_id);

                            restrictions.validate_package_tree(game_io, triplet)
                        });

                    (valid_package, card.clone(), count)
                })
                .map(map_to_item_with_count)
                .filter(|item| item.count != 0)
                .map(Some)
                .collect()
        } else {
            // use all packages for pack
            package_manager
                .packages(NAMESPACE)
                .filter(|package| {
                    !package.hidden && package.card_properties.card_class != CardClass::Recipe
                })
                .flat_map(|package| {
                    let package_info = package.package_info();
                    let triplet = package.package_info.triplet();
                    let valid_package = restrictions.validate_package_tree(game_io, triplet);

                    package.default_codes.iter().map(move |code| {
                        let card = Card {
                            package_id: package_info.id.clone(),
                            code: code.clone(),
                        };

                        (valid_package, card, package.limit)
                    })
                })
                .map(map_to_item_with_count)
                .filter(|item| item.count != 0)
                .map(Some)
                .collect()
        }
    }

    pub fn draw_list_item(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        mode: EditorMode,
        position: Vec2,
    ) {
        let mut color = if self.valid {
            Color::WHITE
        } else {
            Color::ORANGE
        };

        if mode == EditorMode::SelectRegular {
            let regular_allowed = self.valid && {
                let globals = game_io.resource::<Globals>().unwrap();
                let card_packages = &globals.card_packages;

                card_packages
                    .package(NAMESPACE, &self.card.package_id)
                    .is_some_and(|package| package.regular_allowed)
            };

            if !regular_allowed {
                color = color.multiply_color(0.75);
            }
        }

        self.card
            .draw_list_item(game_io, sprite_queue, position, color);

        if !self.show_count {
            return;
        }

        const COUNT_OFFSET: Vec2 = Vec2::new(120.0, 3.0);

        let mut label = Text::new(game_io, FontName::Thick);
        label.style.color = color;

        label.style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        label.style.bounds.set_position(COUNT_OFFSET + position);
        label.text = format!("{:>2}", self.count.min(99));
        label.draw(game_io, sprite_queue);
    }
}
