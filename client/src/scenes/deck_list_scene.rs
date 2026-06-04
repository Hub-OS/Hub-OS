use super::DeckEditorScene;
use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::{Deck, DeckTagKey, GlobalSave};
use framework::prelude::*;
use itertools::Itertools;
use packets::structures::Uuid;

const CHARACTER_LIMIT: usize = 9;

enum Event {
    FilterName(String),
    Rename(String),
    Delete,
    CreateTag(String),
    DeleteTagRequest(DeckTagKey),
    DeleteTag(DeckTagKey),
    RenameTagRequest(DeckTagKey),
    RenameTag(DeckTagKey, String),
}

#[derive(Clone, Copy)]
enum DeckListOption {
    Search,
    FilterTag,
    EditTags,
}

#[derive(PartialEq, Eq)]
enum TagMenuMode {
    TaggingDeck,
    EditingTags,
    Filtering,
}

#[derive(Clone, Copy)]
enum DeckOption {
    Edit,
    Equip,
    ChangeName,
    New,
    More,
    Back,
    Clone,
    Export,
    Import,
    More2,
    Tag,
    Delete,
}

impl DeckOption {
    const FRESH_PAGE: &[(&'static str, DeckOption)] = &[
        ("deck-list-option-new", DeckOption::New),
        ("deck-list-option-import", DeckOption::Import),
    ];

    const PAGE_1: &[(&'static str, DeckOption)] = &[
        ("deck-list-option-edit", DeckOption::Edit),
        ("deck-list-option-equip", DeckOption::Equip),
        ("deck-list-option-change-name", DeckOption::ChangeName),
        ("deck-list-option-new", DeckOption::New),
        ("deck-list-option-more", DeckOption::More),
    ];

    const PAGE_2: &[(&'static str, DeckOption)] = &[
        ("deck-list-option-back", DeckOption::Back),
        ("deck-list-option-clone", DeckOption::Clone),
        ("deck-list-option-export", DeckOption::Export),
        ("deck-list-option-import", DeckOption::Import),
        ("deck-list-option-more", DeckOption::More2),
    ];

    const PAGE_3: &[(&'static str, DeckOption)] = &[
        ("deck-list-option-back", DeckOption::More),
        ("deck-list-option-tag", DeckOption::Tag),
        ("deck-list-option-delete", DeckOption::Delete),
    ];
}

pub struct DeckListScene {
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
    frame: SubSceneFrame,
    ui_input_tracker: UiInputTracker,
    scene_time: FrameTime,
    equipped_sprite: Sprite,
    equipped_animator: Animator,
    regular_card_sprite: Sprite,
    deck_scroll_offset: f32,
    deck_sprite: Sprite,
    deck_name_offset: Vec2,
    deck_frame_sprite: Sprite,
    deck_start_position: Vec2,
    deck_list: DeckList,
    card_scroll_tracker: ScrollTracker,
    card_list_position: Vec2,
    tags_menu: CommandPalette<Option<DeckTagKey>>,
    tag_menu_mode: TagMenuMode,
    deck_context_menu: ContextMenu<DeckOption>,
    list_context_menu: ContextMenu<DeckListOption>,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    textbox: Textbox,
    next_scene: NextScene,
}

impl DeckListScene {
    pub fn new(game_io: &mut GameIO) -> Box<Self> {
        move_selected_deck(game_io);

        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // ui
        let ui_sprite = assets.new_sprite(game_io, ResourcePaths::DECKS_UI);
        let mut ui_animator = Animator::load_new(assets, ResourcePaths::DECKS_UI_ANIMATION);

        // layout points
        ui_animator.set_state("DEFAULT");
        let deck_start_position = ui_animator.point_or_zero("DECK_START");
        let frame_position = ui_animator.point_or_zero("CARD_LIST");

        // regular card sprite
        let mut regular_card_sprite = assets.new_sprite(game_io, ResourcePaths::REGULAR_CARD);
        let mut regular_animator =
            Animator::load_new(assets, ResourcePaths::REGULAR_CARD_ANIMATION);

        regular_animator.set_state("DEFAULT");
        regular_animator.apply(&mut regular_card_sprite);

        // deck sprites
        let mut deck_sprite = ui_sprite.clone();
        ui_animator.set_state("DECK");
        ui_animator.apply(&mut deck_sprite);

        let deck_name_offset = ui_animator.point_or_zero("NAME");

        // card list
        let mut deck_frame_sprite = ui_sprite.clone();
        ui_animator.set_state("CARD_LIST");
        ui_animator.apply(&mut deck_frame_sprite);

        deck_frame_sprite.set_position(frame_position);

        // card scroll tracker
        let mut card_scroll_tracker = ScrollTracker::new(game_io, 5);

        let card_list_position = ui_animator.point_or_zero("LIST") + frame_position;

        let scroll_start = ui_animator.point_or_zero("SCROLL_START") + frame_position;
        let scroll_end = ui_animator.point_or_zero("SCROLL_END") + frame_position;

        card_scroll_tracker.define_scrollbar(scroll_start, scroll_end);

        // deck cursor sprite
        let mut deck_scroll_tracker = ScrollTracker::new(game_io, 3);
        deck_scroll_tracker.use_custom_cursor(
            game_io,
            ResourcePaths::DECKS_CURSOR_ANIMATION,
            ResourcePaths::DECKS_CURSOR,
        );
        deck_scroll_tracker.set_vertical(false);

        let cursor_start = deck_start_position + Vec2::new(3.0, 7.0);
        deck_scroll_tracker.define_cursor(cursor_start, deck_sprite.size().x + 1.0);

        // list context menu
        let mut list_context_menu = ContextMenu::new_translated(
            game_io,
            "deck-list-filter-menu-label",
            Vec2::new(3.0, 50.0),
        )
        .with_translated_options(
            game_io,
            &[
                ("deck-list-option-search", DeckListOption::Search),
                ("deck-list-option-filter-tag", DeckListOption::FilterTag),
                ("deck-list-option-edit-tags", DeckListOption::EditTags),
            ],
        );

        list_context_menu.recalculate_layout(game_io);
        list_context_menu.set_center(RESOLUTION_F * 0.5);

        // equipped sprite
        let equipped_sprite = ui_sprite.clone();
        let mut equipped_animator = ui_animator.clone();
        equipped_animator.set_state("EQUIPPED_BADGE");
        equipped_animator.set_loop_mode(AnimatorLoopMode::Loop);

        let (event_sender, event_receiver) = flume::unbounded();

        Box::new(Self {
            camera,
            background: Background::new_sub_scene(game_io),
            scene_title: SceneTitle::new(game_io, "deck-list-scene-title"),
            frame: SubSceneFrame::new(game_io).with_top_bar(true),
            ui_input_tracker: UiInputTracker::new(),
            scene_time: 0,
            equipped_sprite,
            equipped_animator,
            regular_card_sprite,
            deck_scroll_offset: 0.0,
            deck_sprite,
            deck_name_offset,
            deck_frame_sprite,
            deck_start_position,
            deck_list: DeckList::new(game_io, deck_scroll_tracker),
            card_scroll_tracker,
            card_list_position,
            deck_context_menu: ContextMenu::new_translated(
                game_io,
                "deck-list-context-menu-label",
                Vec2::new(3.0, 50.0),
            )
            .with_arrow(true),
            list_context_menu,
            tags_menu: CommandPalette::new(game_io, "deck-list-tags-menu-label"),
            tag_menu_mode: TagMenuMode::EditingTags,
            event_sender,
            event_receiver,
            textbox: Textbox::new_navigation(game_io),
            next_scene: NextScene::None,
        })
    }
}

fn move_selected_deck(game_io: &mut GameIO) {
    let globals = Globals::from_resources_mut(game_io);
    let save = &mut globals.global_save;

    if save.selected_deck == 0 {
        return;
    }

    let deck = save.decks.remove(save.selected_deck);
    save.decks.insert(0, deck);
    save.selected_deck = 0;
}

fn build_deck_tag_options(game_io: &GameIO, deck_i: usize) -> Vec<(String, Option<DeckTagKey>)> {
    let globals = Globals::from_resources(game_io);
    let global_save = &globals.global_save;

    let deck = &global_save.decks[deck_i];

    let mut options: Vec<_> =
        std::iter::once((globals.translate("deck-list-tags-option-new-tag"), None))
            .chain(global_save.deck_tags.iter().map(|(tag_key, label)| {
                let label = if deck.tags.contains(&tag_key) {
                    format!("- {label}")
                } else {
                    format!("+ {label}")
                };

                (label.clone(), Some(tag_key))
            }))
            .collect();

    options[1..].sort_by(|(label_a, _), (label_b, _)| {
        (label_a.starts_with('+'), label_a).cmp(&(label_b.starts_with('+'), label_b))
    });

    options
}

fn build_tag_filter_options(game_io: &GameIO) -> Vec<(String, Option<DeckTagKey>)> {
    let globals = Globals::from_resources(game_io);
    let global_save = &globals.global_save;

    let mut options: Vec<_> =
        std::iter::once((globals.translate("deck-list-tags-option-no-filter"), None))
            .chain(
                global_save
                    .deck_tags
                    .iter()
                    .map(|(k, v)| (v.clone(), Some(k))),
            )
            .collect();

    options[1..].sort_by(|(label_a, _), (label_b, _)| label_a.cmp(label_b));

    options
}

fn build_tag_edit_options(game_io: &GameIO) -> Vec<(String, Option<DeckTagKey>)> {
    let globals = Globals::from_resources(game_io);
    let global_save = &globals.global_save;

    let mut options: Vec<_> = global_save
        .deck_tags
        .iter()
        .map(|(k, v)| (v.clone(), Some(k)))
        .collect();

    options.sort_by(|(label_a, _), (label_b, _)| label_a.cmp(label_b));

    options
}

impl Scene for DeckListScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        let global_save = &Globals::from_resources(game_io).global_save;
        let decks = &global_save.decks;

        if let Some(deck_i) = self.deck_list.selected_deck_index() {
            let deck = &decks[deck_i];

            // update scroll tracker
            let count = deck.cards.len();
            self.card_scroll_tracker.set_total_items(count);

            // update validity
            self.deck_list.validities[deck_i] = self.deck_list.validate_deck(game_io, deck);
        } else if decks.is_empty() {
            let global_save = &mut Globals::from_resources_mut(game_io).global_save;

            // make sure there's always at least one deck
            global_save.selected_deck = 0;
            create_new_deck(self, game_io);
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.scene_time += 1;
        self.camera.update();
        self.background.update();

        if game_io.is_in_transition() {
            return;
        }

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        self.textbox.update(game_io);

        handle_events(self, game_io);

        if self.textbox.is_open() {
            return;
        }

        handle_input(self, game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let globals = Globals::from_resources(game_io);
        let global_save = &globals.global_save;

        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw selected deck frame
        sprite_queue.draw_sprite(&self.deck_frame_sprite);

        // deck offset calculations
        let target_index = self.deck_list.scroll_tracker.top_index();
        self.deck_scroll_offset += (target_index as f32 - self.deck_scroll_offset) * 0.2;

        let selection_multiplier = self.deck_sprite.size().x + 1.0;
        let deck_offset = -self.deck_scroll_offset * selection_multiplier;

        let visible_start_i = (self.deck_scroll_offset.max(0.0) as usize).saturating_sub(1);
        let deck_view_len = (RESOLUTION_F.x / selection_multiplier) as usize + 3;

        // draw decks
        let mut label =
            TextStyle::new(game_io, FontName::Thick).with_shadow_color(TEXT_DARK_SHADOW_COLOR);

        for (i, &deck_i) in self
            .deck_list
            .visible_indices
            .iter()
            .skip(visible_start_i)
            .take(deck_view_len)
            .enumerate()
        {
            let render_i = i + visible_start_i;

            let mut position = self.deck_start_position;
            position.x += render_i as f32 * selection_multiplier + deck_offset;

            // draw deck first
            self.deck_sprite.set_position(position);
            sprite_queue.draw_sprite(&self.deck_sprite);

            if i == global_save.selected_deck {
                self.equipped_animator.update();
                self.equipped_animator.apply(&mut self.equipped_sprite);

                self.equipped_sprite.set_position(position);
                sprite_queue.draw_sprite(&self.equipped_sprite);
            }

            // deck label
            label.color = if self.deck_list.validities[deck_i].is_valid() {
                Color::WHITE
            } else {
                Color::ORANGE
            };

            let deck = &global_save.decks[deck_i];

            label.bounds.set_position(position + self.deck_name_offset);
            label.draw(game_io, &mut sprite_queue, &deck.name);
        }

        // draw selected deck card list
        if let Some(deck_i) = self.deck_list.selected_deck_index() {
            // draw deck cursor
            self.deck_list.scroll_tracker.draw_cursor(&mut sprite_queue);

            // draw cards
            let deck_validity = &self.deck_list.validities[deck_i];
            let deck = &global_save.decks[deck_i];
            let range = self.card_scroll_tracker.view_range();
            let mut card_position = self.card_list_position;

            for i in range {
                let card = &deck.cards[i];
                let valid = deck_validity.is_card_valid(card);
                let color = if valid { Color::WHITE } else { Color::ORANGE };

                card.draw_list_item(game_io, &mut sprite_queue, card_position, color);

                if deck.regular_index == Some(i) {
                    self.regular_card_sprite.set_position(card_position);
                    sprite_queue.draw_sprite(&self.regular_card_sprite);
                }

                card_position.y += 16.0;
            }

            // draw scrollbar
            self.card_scroll_tracker.draw_scrollbar(&mut sprite_queue);
        }

        // draw frame and title
        self.frame.draw(&mut sprite_queue);
        self.scene_title.draw(game_io, &mut sprite_queue);

        // draw context menus
        self.deck_context_menu.draw(game_io, &mut sprite_queue);
        self.list_context_menu.draw(game_io, &mut sprite_queue);

        // draw tags menu
        self.tags_menu.draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn handle_events(scene: &mut DeckListScene, game_io: &mut GameIO) {
    let globals = Globals::from_resources_mut(game_io);
    let global_save = &mut globals.global_save;
    let Ok(event) = scene.event_receiver.try_recv() else {
        return;
    };

    match event {
        Event::FilterName(name_filter) => {
            scene.deck_list.name_filter = name_filter.to_lowercase();
            scene.deck_list.apply_filters(game_io);
        }
        Event::Rename(name) => {
            let deck_index = scene.deck_list.selected_deck_index().unwrap();
            scene.deck_list.searchable_names[deck_index] = name.to_lowercase();

            let deck = &mut global_save.decks[deck_index];
            deck.name = name;
            deck.update_time = GlobalSave::current_time();
            global_save.save();

            scene.textbox.close();
        }
        Event::Delete => {
            let deck_index = scene.deck_list.remove_selected().unwrap();
            global_save.decks.remove(deck_index);

            if global_save.selected_deck == deck_index {
                global_save.selected_deck = 0;
                global_save.selected_deck_time = GlobalSave::current_time();
            } else if global_save.selected_deck > deck_index {
                global_save.selected_deck -= 1;
            }

            global_save.save();

            // update card list
            let card_count = match scene.deck_list.selected_deck_index() {
                Some(deck_index) => global_save.decks[deck_index].cards.len(),
                None => 0,
            };

            scene.card_scroll_tracker.set_total_items(card_count);
            scene.textbox.close();

            // make sure there's always at least one deck
            if global_save.decks.is_empty() {
                create_new_deck(scene, game_io);
            }
        }
        Event::CreateTag(label) => {
            if !label.is_empty() && !global_save.deck_tags.values().any(|l| *l == label) {
                let tag_key = global_save.deck_tags.insert(label);

                if scene.tag_menu_mode == TagMenuMode::TaggingDeck {
                    // apply tag to the selected deck
                    let deck_i = scene.deck_list.selected_deck_index().unwrap();
                    let deck = &mut global_save.decks[deck_i];
                    deck.tags.push(tag_key);
                }

                global_save.save();

                // update list
                let options = if scene.tag_menu_mode == TagMenuMode::TaggingDeck {
                    let deck_i = scene.deck_list.selected_deck_index().unwrap();
                    build_deck_tag_options(game_io, deck_i)
                } else {
                    build_tag_filter_options(game_io)
                };

                scene.tags_menu.set_options(options);
                scene.textbox.close();
            }
        }
        Event::DeleteTagRequest(tag_key) => {
            let tag_label = &global_save.deck_tags[tag_key];
            let label_arg = tag_label.clone().into();
            let question =
                globals.translate_with_args("deck-list-delete-question", vec![("name", label_arg)]);

            let textbox = &mut scene.textbox;
            let event_sender = scene.event_sender.clone();
            textbox.push_interface(TextboxQuestion::new(game_io, question, move |yes| {
                if yes {
                    let event = Event::DeleteTag(tag_key);
                    let _ = event_sender.send(event);
                }
            }));
            textbox.open();
        }
        Event::DeleteTag(tag_key) => {
            global_save.deck_tags.remove(tag_key);

            // remove tag from all decks
            for deck in &mut global_save.decks {
                if let Some(i) = deck.tags.iter().position(|t| *t == tag_key) {
                    deck.tags.swap_remove(i);
                }
            }

            global_save.save();

            let tag_options = build_tag_edit_options(game_io);
            scene.tags_menu.set_options(tag_options);
        }
        Event::RenameTagRequest(tag_key) => {
            let tag_label = &global_save.deck_tags[tag_key];

            let textbox = &mut scene.textbox;
            let event_sender = scene.event_sender.clone();
            textbox.push_interface(
                TextboxPrompt::new(move |new_label| {
                    let event = Event::RenameTag(tag_key, new_label);
                    let _ = event_sender.send(event);
                })
                .with_str(tag_label),
            );
            textbox.open();
        }
        Event::RenameTag(tag_key, label) => {
            if !label.is_empty() && !global_save.deck_tags.values().contains(&label) {
                global_save.deck_tags[tag_key] = label;
                global_save.save();

                let tag_options = build_tag_edit_options(game_io);
                scene.tags_menu.set_options(tag_options);
            }
        }
    }
}

fn handle_input(scene: &mut DeckListScene, game_io: &mut GameIO) {
    scene.ui_input_tracker.update(game_io);

    if scene.deck_context_menu.is_open() {
        handle_deck_context_menu_input(scene, game_io);
        return;
    }

    if scene.list_context_menu.is_open() {
        handle_list_context_menu_input(scene, game_io);
        return;
    }

    if scene.tags_menu.is_open() {
        handle_tags_menu_input(scene, game_io);
        return;
    }

    // deck scroll
    let total_visible_decks = scene.deck_list.scroll_tracker.total_items();

    if total_visible_decks > 0 {
        let previous_deck_index = scene.deck_list.selected_deck_index();

        if scene.ui_input_tracker.pulsed(Input::Left) {
            scene.deck_list.scroll_tracker.move_up();
        }

        if scene.ui_input_tracker.pulsed(Input::Right) {
            scene.deck_list.scroll_tracker.move_down();
        }

        let deck_index = scene.deck_list.selected_deck_index();

        if previous_deck_index != deck_index {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);

            let decks = &globals.global_save.decks;
            let count = decks[deck_index.unwrap()].cards.len();
            scene.card_scroll_tracker.set_total_items(count);
        }
    }

    // card scroll
    let previous_card_index = scene.card_scroll_tracker.selected_index();

    if scene.ui_input_tracker.pulsed(Input::Up) {
        scene.card_scroll_tracker.move_view_up();
    }

    if scene.ui_input_tracker.pulsed(Input::Down) {
        scene.card_scroll_tracker.move_view_down();
    }

    if scene.ui_input_tracker.pulsed(Input::ShoulderL) {
        scene.card_scroll_tracker.page_up();
    }

    if scene.ui_input_tracker.pulsed(Input::ShoulderR) {
        scene.card_scroll_tracker.page_down();
    }

    if previous_card_index != scene.card_scroll_tracker.selected_index() {
        let globals = Globals::from_resources(game_io);
        globals.audio.play_sound(&globals.sfx.cursor_move);
    }

    // confirm + cancel
    let input_util = InputUtil::new(game_io);

    if input_util.was_just_pressed(Input::Cancel) {
        let globals = Globals::from_resources(game_io);
        globals.audio.play_sound(&globals.sfx.cursor_cancel);

        let transition = crate::transitions::new_scene_pop(game_io);
        scene.next_scene = NextScene::new_pop().with_transition(transition);
        return;
    }

    if input_util.was_just_pressed(Input::Confirm) {
        let globals = Globals::from_resources(game_io);
        globals.audio.play_sound(&globals.sfx.cursor_select);

        let context_menu = &mut scene.deck_context_menu;

        if total_visible_decks == 0 {
            context_menu.set_and_translate_options(game_io, DeckOption::FRESH_PAGE);
        } else {
            context_menu.set_and_translate_options(game_io, DeckOption::PAGE_1);
        }

        scene.deck_context_menu.open();
    } else if input_util.was_just_pressed(Input::Option2) {
        let globals = Globals::from_resources(game_io);
        globals.audio.play_sound(&globals.sfx.cursor_select);
        scene.list_context_menu.open();
    }
}

fn handle_deck_context_menu_input(scene: &mut DeckListScene, game_io: &mut GameIO) {
    let Some(selection) = scene
        .deck_context_menu
        .update(game_io, &scene.ui_input_tracker)
    else {
        return;
    };

    let globals = Globals::from_resources_mut(game_io);

    match selection {
        DeckOption::Edit => {
            let deck_index = scene.deck_list.selected_deck_index().unwrap();

            scene.next_scene = NextScene::new_push(DeckEditorScene::new(game_io, deck_index))
                .with_transition(crate::transitions::new_sub_scene(game_io));
        }
        DeckOption::Equip => {
            let global_save = &mut globals.global_save;
            global_save.selected_deck = scene.deck_list.selected_deck_index().unwrap();
            global_save.selected_deck_time = GlobalSave::current_time();
            global_save.save();
        }
        DeckOption::ChangeName => {
            let event_sender = scene.event_sender.clone();
            let callback = move |name: String| {
                if name.is_empty() {
                    return;
                }

                let event = Event::Rename(name);
                event_sender.send(event).unwrap();
            };

            let index = scene.deck_list.selected_deck_index().unwrap();
            let global_save = &globals.global_save;
            let deck_name = &global_save.decks[index].name;
            let textbox_interface = TextboxPrompt::new(callback)
                .with_str(deck_name)
                .with_character_limit(CHARACTER_LIMIT);

            scene.textbox.push_interface(textbox_interface);
            scene.textbox.open();
        }
        DeckOption::New => create_new_deck(scene, game_io),
        DeckOption::More => {
            let context_menu = &mut scene.deck_context_menu;
            context_menu.set_and_translate_options(game_io, DeckOption::PAGE_2);
        }
        DeckOption::More2 => {
            let context_menu = &mut scene.deck_context_menu;
            context_menu.set_and_translate_options(game_io, DeckOption::PAGE_3);
        }
        DeckOption::Back => {
            let context_menu = &mut scene.deck_context_menu;
            context_menu.set_and_translate_options(game_io, DeckOption::PAGE_1);
            context_menu.set_selected_index(DeckOption::PAGE_1.len() - 1);
        }
        DeckOption::Clone => {
            let global_save = &mut globals.global_save;

            let index = scene.deck_list.selected_deck_index().unwrap();

            // clone deck
            let mut deck = global_save.decks[index].clone();
            deck.uuid = Uuid::new_v4();
            let card_len = deck.cards.len();

            // clone validity
            let validity = scene.deck_list.validities[index].clone();

            // insert deck
            let insert_index = scene.deck_list.prepare_insert(validity, &deck.name);
            global_save.decks.insert(insert_index, deck);

            // update scroll tracker
            scene.card_scroll_tracker.set_total_items(card_len);

            global_save.save();
        }
        DeckOption::Export => {
            let globals = Globals::from_resources(game_io);

            let index = scene.deck_list.selected_deck_index().unwrap();
            let deck = &globals.global_save.decks[index];
            let text = deck.export_string(game_io);
            let copied = game_io.input_mut().clipboard_mut().set_text(text);

            let globals = Globals::from_resources(game_io);
            let message = if copied {
                globals.translate("copied-to-clipboard")
            } else {
                globals.translate("copy-to-clipboard-failed")
            };
            let textbox_interface = TextboxMessage::new(message);

            scene.textbox.push_interface(textbox_interface);
            scene.textbox.open();
        }
        DeckOption::Import => {
            let text = game_io.input_mut().clipboard_mut().request_text();

            if let Some(mut deck) = Deck::import_string(&text) {
                let globals = Globals::from_resources(game_io);

                if deck.name.is_empty() {
                    deck.name = globals.translate("deck-list-new-deck-name");
                }

                // validate deck
                let validity = scene.deck_list.validate_deck(game_io, &deck);
                let insert_index = scene.deck_list.prepare_insert(validity, &deck.name);

                // update total cards for the card scroll tracker
                scene.card_scroll_tracker.set_total_items(deck.cards.len());

                // save deck
                let globals = Globals::from_resources_mut(game_io);
                let decks = &mut globals.global_save.decks;
                decks.insert(insert_index, deck);
            } else {
                let globals = Globals::from_resources(game_io);

                let message = globals.translate("clipboard-read-failed");
                let textbox_interface = TextboxMessage::new(message);

                scene.textbox.push_interface(textbox_interface);
                scene.textbox.open();
            }
        }
        DeckOption::Tag => {
            let deck_i = scene.deck_list.selected_deck_index().unwrap();
            let tag_options = build_deck_tag_options(game_io, deck_i);
            scene.tags_menu.set_options(tag_options);
            scene.tags_menu.open();
            scene.tag_menu_mode = TagMenuMode::TaggingDeck;
        }
        DeckOption::Delete => {
            let global_save = &globals.global_save;

            let index = scene.deck_list.selected_deck_index().unwrap();
            let deck_name = &global_save.decks[index].name;

            let question = globals.translate_with_args(
                "deck-list-delete-question",
                vec![("name", deck_name.into())],
            );

            let event_sender = scene.event_sender.clone();
            let textbox_interface = TextboxQuestion::new(game_io, question, move |response| {
                if !response {
                    return;
                }

                let event = Event::Delete;
                event_sender.send(event).unwrap();
            });

            scene.textbox.push_interface(textbox_interface);
            scene.textbox.open();
        }
    }

    if !matches!(
        selection,
        DeckOption::More | DeckOption::More2 | DeckOption::Back
    ) {
        scene.deck_context_menu.close();
    }
}

fn handle_list_context_menu_input(scene: &mut DeckListScene, game_io: &mut GameIO) {
    let Some(selection) = scene
        .list_context_menu
        .update(game_io, &scene.ui_input_tracker)
    else {
        return;
    };

    match selection {
        DeckListOption::Search => {
            let event_sender = scene.event_sender.clone();
            let interface = TextboxPrompt::new(move |name| {
                let _ = event_sender.send(Event::FilterName(name));
            })
            .with_character_limit(CHARACTER_LIMIT);

            scene.textbox.push_interface(interface);
            scene.textbox.open();
        }
        DeckListOption::FilterTag => {
            let tag_options = build_tag_filter_options(game_io);
            scene.tags_menu.set_options(tag_options);
            scene.tags_menu.open();
            scene.tag_menu_mode = TagMenuMode::Filtering;
        }
        DeckListOption::EditTags => {
            let tag_options = build_tag_edit_options(game_io);
            scene.tags_menu.set_options(tag_options);
            scene.tags_menu.open();
            scene.tag_menu_mode = TagMenuMode::EditingTags;
        }
    }

    scene.list_context_menu.close();
}

fn handle_tags_menu_input(scene: &mut DeckListScene, game_io: &mut GameIO) {
    let Some(selection) = scene.tags_menu.update(game_io, &scene.ui_input_tracker) else {
        if scene.tag_menu_mode == TagMenuMode::TaggingDeck && !scene.tags_menu.is_open() {
            // save deck tag changes after closing the tags menu to avoid saving too frequently
            // the other modes save on every change
            let globals = Globals::from_resources_mut(game_io);
            globals.global_save.save();
        }

        return;
    };

    match scene.tag_menu_mode {
        TagMenuMode::TaggingDeck => {
            let Some(tag_key) = selection else {
                create_new_tag_prompt(scene);
                return;
            };

            let globals = Globals::from_resources_mut(game_io);
            let decks = &mut globals.global_save.decks;
            let Some(deck_i) = scene.deck_list.selected_deck_index() else {
                return;
            };

            let Some(deck) = decks.get_mut(deck_i) else {
                return;
            };

            match deck.tags.iter().position(|k| *k == tag_key) {
                Some(tag_i) => {
                    // remove the tag from the deck
                    deck.tags.swap_remove(tag_i);

                    // no need to select the next row, automatically occurs as everything shifts up
                }
                None => {
                    // add the tag to the deck
                    deck.tags.push(tag_key);

                    // select the next row, feels more natural as we are adding an element above when building the list
                    let index = scene.tags_menu.selected_index();
                    scene.tags_menu.set_selected_index(index + 1);
                }
            }

            let tag_options = build_deck_tag_options(game_io, deck_i);
            scene.tags_menu.set_options(tag_options);
        }
        TagMenuMode::EditingTags => {
            let Some(tag_key) = selection else {
                create_new_tag_prompt(scene);
                return;
            };

            let globals = Globals::from_resources(game_io);

            let textbox = &mut scene.textbox;
            let event_sender = scene.event_sender.clone();

            let options: &[&str; 3] = &[
                &format!("\x04{}", globals.translate("deck-list-tags-option-rename")),
                &format!("\x04{}", globals.translate("deck-list-tags-option-delete")),
                &format!("\x04{}", globals.translate("deck-list-tags-option-cancel")),
            ];

            textbox.push_interface(
                TextboxQuiz::new(options, move |selection| {
                    let event = match selection {
                        0 => Some(Event::RenameTagRequest(tag_key)),
                        1 => Some(Event::DeleteTagRequest(tag_key)),
                        _ => None,
                    };

                    if let Some(event) = event {
                        let _ = event_sender.send(event);
                    }
                })
                .with_cancel_response(2),
            );
            textbox.open();
        }
        TagMenuMode::Filtering => {
            scene.deck_list.tag_filter = selection;
            scene.deck_list.apply_filters(game_io);
            scene.tags_menu.close();
        }
    }
}

fn create_new_tag_prompt(scene: &mut DeckListScene) {
    let textbox = &mut scene.textbox;
    let event_sender = scene.event_sender.clone();

    textbox.push_interface(TextboxPrompt::new(move |label| {
        let _ = event_sender.send(Event::CreateTag(label));
    }));
    textbox.open();
}

fn create_new_deck(scene: &mut DeckListScene, game_io: &mut GameIO) {
    let globals = Globals::from_resources_mut(game_io);
    let name = globals.translate("deck-list-new-deck-name");

    let global_save = &mut globals.global_save;
    let insert_index = scene.deck_list.prepare_insert(Default::default(), &name);
    global_save.decks.insert(insert_index, Deck::new(name));

    scene.card_scroll_tracker.set_total_items(0);
}

struct DeckList {
    deck_restrictions: DeckRestrictions,
    validities: Vec<DeckValidity>,
    searchable_names: Vec<String>,
    scroll_tracker: ScrollTracker,
    visible_indices: Vec<usize>,
    name_filter: String,
    tag_filter: Option<DeckTagKey>,
}

impl DeckList {
    fn new(game_io: &GameIO, scroll_tracker: ScrollTracker) -> Self {
        let globals = Globals::from_resources(game_io);

        // deck validities
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

        let deck_validities = (globals.global_save.decks)
            .iter()
            .map(|deck| deck_restrictions.validate_deck(game_io, PackageNamespace::Local, deck))
            .collect();

        let searchable_names = (globals.global_save.decks)
            .iter()
            .map(|deck| deck.name.to_lowercase())
            .collect();

        Self {
            deck_restrictions,
            validities: deck_validities,
            searchable_names,
            scroll_tracker: scroll_tracker.with_total_items(global_save.decks.len()),
            visible_indices: (0..global_save.decks.len()).collect(),
            name_filter: Default::default(),
            tag_filter: None,
        }
    }

    fn selected_deck_index(&self) -> Option<usize> {
        self.visible_indices
            .get(self.scroll_tracker.selected_index())
            .copied()
    }

    fn validate_deck(&self, game_io: &GameIO, deck: &Deck) -> DeckValidity {
        self.deck_restrictions
            .validate_deck(game_io, PackageNamespace::Local, deck)
    }

    fn prepare_insert(&mut self, validity: DeckValidity, name: &str) -> usize {
        let index = self.scroll_tracker.selected_index();

        let mut new_deck_i = 0;
        let mut new_index = 0;

        if !self.visible_indices.is_empty() {
            // shift everything
            for i in &mut self.visible_indices[index + 1..] {
                *i += 1;
            }

            // insert after the current deck
            let deck_i = self.visible_indices[index];
            new_deck_i = deck_i + 1;
            new_index = index + 1;
        }

        // make the new deck visible
        self.visible_indices.insert(new_index, new_deck_i);

        // insert validity
        self.validities.insert(new_deck_i, validity);

        // insert name
        self.searchable_names
            .insert(new_deck_i, name.to_lowercase());

        // update scroll tracker
        let visible_len = self.visible_indices.len();
        self.scroll_tracker.set_total_items(visible_len);
        self.scroll_tracker.set_selected_index(new_index);

        new_deck_i
    }

    fn remove_selected(&mut self) -> Option<usize> {
        let deck_i = self.selected_deck_index()?;

        let index = self.scroll_tracker.selected_index();
        self.visible_indices.remove(index);
        self.validities.remove(deck_i);
        self.searchable_names.remove(deck_i);

        // shift everything
        for i in &mut self.visible_indices[index..] {
            *i -= 1;
        }

        let visible_len = self.visible_indices.len();
        self.scroll_tracker.set_total_items(visible_len);

        Some(deck_i)
    }

    fn apply_filters(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);
        let global_save = &globals.global_save;

        self.visible_indices.clear();
        self.visible_indices.extend(0..global_save.decks.len());

        if !self.name_filter.is_empty() {
            self.visible_indices
                .retain(|i| self.searchable_names[*i].contains(&self.name_filter));
        }

        if let Some(tag_filter) = self.tag_filter {
            self.visible_indices
                .retain(|i| global_save.decks[*i].tags.contains(&tag_filter));
        }

        let visible_len = self.visible_indices.len();
        self.scroll_tracker.set_total_items(visible_len);
        self.scroll_tracker.set_selected_index(0);
    }
}
