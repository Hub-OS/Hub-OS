use super::DeckEditorScene;
use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::{Deck, GlobalSave};
use framework::prelude::*;
use packets::structures::Uuid;

enum Event {
    Rename(String),
    Delete,
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
    Delete,
}

impl DeckOption {
    const FRESH_PAGE: &[(&'static str, DeckOption)] = &[
        ("deck-list-option-new", DeckOption::New),
        #[cfg(not(target_os = "android"))]
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
        #[cfg(not(target_os = "android"))]
        ("deck-list-option-export", DeckOption::Export),
        #[cfg(not(target_os = "android"))]
        ("deck-list-option-import", DeckOption::Import),
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
    deck_restrictions: DeckRestrictions,
    deck_validities: Vec<DeckValidity>,
    deck_scroll_tracker: ScrollTracker,
    card_scroll_tracker: ScrollTracker,
    card_list_position: Vec2,
    context_menu: ContextMenu<DeckOption>,
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

        // equipped sprite
        let equipped_sprite = ui_sprite.clone();
        let mut equipped_animator = ui_animator.clone();
        equipped_animator.set_state("EQUIPPED_BADGE");
        equipped_animator.set_loop_mode(AnimatorLoopMode::Loop);

        let (event_sender, event_receiver) = flume::unbounded();

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
            deck_restrictions,
            deck_validities,
            deck_scroll_tracker,
            card_scroll_tracker,
            card_list_position,
            context_menu: ContextMenu::new_translated(
                game_io,
                "deck-list-context-menu-label",
                Vec2::new(3.0, 50.0),
            )
            .with_arrow(true),
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

impl Scene for DeckListScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        let global_save = &Globals::from_resources(game_io).global_save;
        let decks = &global_save.decks;

        self.deck_scroll_tracker.set_total_items(decks.len());

        let selected_index = self.deck_scroll_tracker.selected_index();

        if let Some(deck) = decks.get(selected_index) {
            // update scroll tracker
            let count = deck.cards.len();
            self.card_scroll_tracker.set_total_items(count);

            // update validity
            self.deck_validities[selected_index] =
                self.deck_restrictions
                    .validate_deck(game_io, PackageNamespace::Local, deck);
        } else {
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
        let deck_top_index = self.deck_scroll_tracker.top_index();
        self.deck_scroll_offset += (deck_top_index as f32 - self.deck_scroll_offset) * 0.2;

        // draw decks
        let mut label =
            TextStyle::new(game_io, FontName::Thick).with_shadow_color(TEXT_DARK_SHADOW_COLOR);

        let selection_multiplier = self.deck_sprite.size().x + 1.0;

        let deck_offset = -self.deck_scroll_offset * selection_multiplier;

        for (i, deck) in global_save.decks.iter().enumerate() {
            let mut position = self.deck_start_position;
            position.x += i as f32 * selection_multiplier + deck_offset;

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
            label.color = if self.deck_validities[i].is_valid() {
                Color::WHITE
            } else {
                Color::ORANGE
            };

            label.bounds.set_position(position + self.deck_name_offset);
            label.draw(game_io, &mut sprite_queue, &deck.name);
        }

        // draw selected deck card list
        if !global_save.decks.is_empty() {
            // draw deck cursor
            self.deck_scroll_tracker.draw_cursor(&mut sprite_queue);

            // draw cards
            let deck_index = self.deck_scroll_tracker.selected_index();
            let deck_validity = &self.deck_validities[deck_index];
            let deck = &global_save.decks[deck_index];
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

        // draw context menu
        self.context_menu.draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn handle_events(scene: &mut DeckListScene, game_io: &mut GameIO) {
    let global_save = &mut Globals::from_resources_mut(game_io).global_save;

    match scene.event_receiver.try_recv() {
        Ok(Event::Rename(name)) => {
            let deck_index = scene.deck_scroll_tracker.selected_index();
            let deck = &mut global_save.decks[deck_index];
            deck.name = name;
            deck.update_time = GlobalSave::current_time();
            global_save.save();

            scene.textbox.close();
        }
        Ok(Event::Delete) => {
            let deck_index = scene.deck_scroll_tracker.selected_index();
            global_save.decks.remove(deck_index);
            scene.deck_validities.remove(deck_index);

            let new_total = global_save.decks.len();
            scene.deck_scroll_tracker.set_total_items(new_total);

            if global_save.selected_deck <= new_total {
                global_save.selected_deck = 0;
                global_save.selected_deck_time = GlobalSave::current_time();
            }

            global_save.save();

            // update card list
            let deck_index = scene.deck_scroll_tracker.selected_index();
            let card_count = match global_save.decks.get(deck_index) {
                Some(deck) => deck.cards.len(),
                None => 0,
            };

            scene.card_scroll_tracker.set_total_items(card_count);
            scene.textbox.close();

            // make sure there's always at least one deck
            if new_total == 0 {
                create_new_deck(scene, game_io);
            }
        }
        Err(_) => {}
    }
}

fn handle_input(scene: &mut DeckListScene, game_io: &mut GameIO) {
    scene.ui_input_tracker.update(game_io);

    if scene.context_menu.is_open() {
        handle_context_menu_input(scene, game_io);
        return;
    }

    let globals = Globals::from_resources(game_io);

    // deck scroll
    let decks = &globals.global_save.decks;
    let total_decks = decks.len();

    if total_decks > 0 {
        let previous_deck_index = scene.deck_scroll_tracker.selected_index();

        if scene.ui_input_tracker.pulsed(Input::Left) {
            scene.deck_scroll_tracker.move_up();
        }

        if scene.ui_input_tracker.pulsed(Input::Right) {
            scene.deck_scroll_tracker.move_down();
        }

        let deck_index = scene.deck_scroll_tracker.selected_index();

        if previous_deck_index != deck_index {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);

            let count = decks[deck_index].cards.len();
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

        let context_menu = &mut scene.context_menu;

        if total_decks == 0 {
            context_menu.set_and_translate_options(game_io, DeckOption::FRESH_PAGE);
        } else {
            context_menu.set_and_translate_options(game_io, DeckOption::PAGE_1);
        }

        scene.context_menu.open();
    }
}

fn handle_context_menu_input(scene: &mut DeckListScene, game_io: &mut GameIO) {
    let Some(selection) = scene.context_menu.update(game_io, &scene.ui_input_tracker) else {
        return;
    };

    let globals = Globals::from_resources_mut(game_io);

    match selection {
        DeckOption::Edit => {
            let deck_index = scene.deck_scroll_tracker.selected_index();

            scene.next_scene = NextScene::new_push(DeckEditorScene::new(game_io, deck_index))
                .with_transition(crate::transitions::new_sub_scene(game_io));
        }
        DeckOption::Equip => {
            let global_save = &mut globals.global_save;
            global_save.selected_deck = scene.deck_scroll_tracker.selected_index();
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

            let index = scene.deck_scroll_tracker.selected_index();
            let global_save = &globals.global_save;
            let deck_name = &global_save.decks[index].name;
            let textbox_interface = TextboxPrompt::new(callback)
                .with_str(deck_name)
                .with_character_limit(9);

            scene.textbox.push_interface(textbox_interface);
            scene.textbox.open();
        }
        DeckOption::New => create_new_deck(scene, game_io),
        DeckOption::More => {
            let context_menu = &mut scene.context_menu;
            context_menu.set_and_translate_options(game_io, DeckOption::PAGE_2);
        }
        DeckOption::Back => {
            let context_menu = &mut scene.context_menu;
            context_menu.set_and_translate_options(game_io, DeckOption::PAGE_1);
            context_menu.set_selected_index(DeckOption::PAGE_1.len() - 1);
        }
        DeckOption::Clone => {
            let global_save = &mut globals.global_save;

            let index = scene.deck_scroll_tracker.selected_index();

            // clone deck
            let mut deck = global_save.decks[index].clone();
            deck.uuid = Uuid::new_v4();
            global_save.decks.insert(index + 1, deck);

            // clone validity
            let deck_validity = scene.deck_validities[index].clone();
            scene.deck_validities.insert(index + 1, deck_validity);

            // update scroll tracker
            let len = global_save.decks.len();
            scene.deck_scroll_tracker.set_total_items(len);

            global_save.save();
        }
        DeckOption::Export => {
            let globals = Globals::from_resources(game_io);

            let index = scene.deck_scroll_tracker.selected_index();
            let deck = &globals.global_save.decks[index];
            let text = deck.export_string(game_io);
            let copied = game_io.input_mut().set_clipboard_text(text);

            let globals = Globals::from_resources(game_io);
            let message = if copied {
                globals.translate("deck-list-copied-to-clipboard")
            } else {
                globals.translate("deck-list-copied-to-clipboard-failed")
            };
            let textbox_interface = TextboxMessage::new(message);

            scene.textbox.push_interface(textbox_interface);
            scene.textbox.open();
        }
        DeckOption::Import => {
            let next_index = scene.deck_scroll_tracker.selected_index() + 1;
            let insert_index = globals.global_save.decks.len().min(next_index);

            let text = game_io.input_mut().request_clipboard_text();

            if let Some(mut deck) = Deck::import_string(text) {
                let globals = Globals::from_resources(game_io);

                deck.name = globals.translate("deck-list-new-deck-name");

                // validate deck
                let deck_restrictions = globals.restrictions.base_deck_restrictions();
                let ns = PackageNamespace::Local;
                let validity = deck_restrictions.validate_deck(game_io, ns, &deck);
                scene.deck_validities.insert(insert_index, validity);

                // update total cards for the card scroll tracker
                scene.card_scroll_tracker.set_total_items(deck.cards.len());

                // save deck
                let globals = Globals::from_resources_mut(game_io);
                let decks = &mut globals.global_save.decks;
                decks.insert(insert_index, deck);

                // update deck scroll tracker
                scene.deck_scroll_tracker.set_total_items(decks.len());
                scene.deck_scroll_tracker.set_selected_index(insert_index);
            } else {
                let globals = Globals::from_resources(game_io);

                let message = globals.translate("deck-list-import-failed");
                let textbox_interface = TextboxMessage::new(message);

                scene.textbox.push_interface(textbox_interface);
                scene.textbox.open();
            }
        }
        DeckOption::Delete => {
            let global_save = &globals.global_save;

            let index = scene.deck_scroll_tracker.selected_index();
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

    if !matches!(selection, DeckOption::More | DeckOption::Back) {
        scene.context_menu.close();
    }
}

fn create_new_deck(scene: &mut DeckListScene, game_io: &mut GameIO) {
    let globals = Globals::from_resources_mut(game_io);
    let name = globals.translate("deck-list-new-deck-name");

    let global_save = &mut globals.global_save;

    let deck_scroll_tracker = &mut scene.deck_scroll_tracker;
    let next_index = deck_scroll_tracker.selected_index() + 1;
    let insert_index = global_save.decks.len().min(next_index);

    global_save.decks.insert(insert_index, Deck::new(name));
    scene.deck_validities.push(DeckValidity::default());

    let total_decks = global_save.decks.len();

    deck_scroll_tracker.set_total_items(total_decks);
    deck_scroll_tracker.set_selected_index(insert_index);

    scene.card_scroll_tracker.set_total_items(0);
}
