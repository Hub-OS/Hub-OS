use super::State;
use crate::battle::*;
use crate::bindable::*;
use crate::ease::inverse_lerp;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::scenes::BattleEvent;
use framework::prelude::*;

const FORM_FADE_DELAY: FrameTime = 10;
const FORM_FADE_TIME: FrameTime = 20;
const CARD_COLS: usize = CARD_SELECT_CARD_COLS;

#[derive(Clone, Default)]
struct Selection {
    col: i32,
    row: i32,
    form_row: usize,
    has_special_button: bool,
    form_select_time: Option<FrameTime>,
    form_open_time: Option<FrameTime>,
    confirm_time: FrameTime,
    animating_slide: bool,
    recipe_animation: Option<CardRecipeAnimation>,
    erased: bool,
    local: bool,
}

impl Selection {
    fn new() -> Self {
        Self {
            col: 0,
            row: 0,
            form_row: 0,
            has_special_button: false,
            form_select_time: Some(0),
            form_open_time: Some(0),
            confirm_time: 0,
            animating_slide: false,
            recipe_animation: None,
            erased: false,
            local: true,
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum SelectedItem {
    Card(usize),
    Button(usize),
    Confirm,
    None,
}

impl SelectedItem {
    fn button(self, player: &Player) -> Option<&CardSelectButton> {
        match self {
            SelectedItem::Button(index) => PlayerOverridables::card_button_slots_for(player)
                .and_then(|buttons| buttons.get(index)?.as_ref()),
            _ => None,
        }
    }
}

#[derive(Clone)]
pub struct CardSelectState {
    ui: CardSelectUi,
    player_selections: Vec<Selection>,
    time: FrameTime,
    completed: bool,
}

impl State for CardSelectState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, _: &GameIO) -> Option<Box<dyn State>> {
        if self.completed {
            Some(Box::new(FormActivateState::new()))
        } else {
            None
        }
    }

    fn update(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        if self.time == 0 {
            simulation.statistics.turns += 1;
            simulation.turn_gauge.set_time(0);

            // reset staged items confirmation and initialize selection data
            for (_, player) in simulation.entities.query_mut::<&mut Player>() {
                player.staged_items.set_confirmed(false);

                if player.index >= self.player_selections.len() {
                    self.player_selections
                        .resize_with(player.index + 1, Selection::default);
                }

                // initialize selection
                let selection = &mut self.player_selections[player.index];
                selection.local = player.local;
                selection.animating_slide = true;
            }

            simulation.update_components(game_io, resources, ComponentLifetime::CardSelectOpen);

            // handle frame 0 scripted confirmation
            let mut pre_confirmed = false;
            for (_, player) in simulation.entities.query_mut::<&mut Player>() {
                if !player.staged_items.confirmed() {
                    continue;
                }

                if player.local {
                    pre_confirmed = true;
                }

                // update data
                let selection = &mut self.player_selections[player.index];
                selection.confirm_time = -CardSelectUi::SLIDE_DURATION;
                selection.recipe_animation =
                    CardRecipeAnimation::try_new(game_io, resources, player);
            }

            // sfx
            if !pre_confirmed {
                let globals = game_io.resource::<Globals>().unwrap();
                simulation.play_sound(game_io, &globals.sfx.card_select_open);
            }

            self.dark_card_check(simulation, game_io);
        }

        CardSelectButton::update_all_from_staged_items(game_io, resources, simulation);

        for selection in &mut self.player_selections {
            // assume erased until we hit the next loop
            selection.erased = true;
        }

        // we'll only see our own selection
        // but we must simulate every player's input to stay in sync
        let id_index_tuples = simulation
            .entities
            .query_mut::<&Player>()
            .into_iter()
            .map(|(id, player)| (id.into(), player.index))
            .collect::<Vec<(EntityId, usize)>>();

        for (entity_id, player_index) in id_index_tuples {
            let selection = &mut self.player_selections[player_index];

            // unmark as erased
            selection.erased = false;

            if selection.animating_slide {
                // don't update if we're animating slide
                continue;
            }

            // update recipe animation if it's visible
            if let Some(animation) = &mut selection.recipe_animation {
                animation.update(game_io, simulation, resources, selection.local);

                if animation.completed() {
                    animation.apply(game_io, simulation, resources, entity_id);
                    selection.recipe_animation = None;
                }
            }

            if selection.confirm_time != 0 {
                // player confirmed selection, we can skip input
                continue;
            }

            self.handle_input(game_io, resources, simulation, entity_id, player_index);
        }

        // animate ui

        for selection in &mut self.player_selections {
            // resolve the slide animation state of all players to stay in sync
            let slide_progress = self.ui.slide_progress(selection.confirm_time);

            selection.animating_slide = if selection.confirm_time == 0 {
                slide_progress < 1.0
            } else {
                // animation is reversed when every player has confirmed
                slide_progress > 0.0
            };

            if !selection.local {
                continue;
            }

            // update the ui for just the local player

            self.ui.animate_slide(simulation, selection.confirm_time);
            self.ui
                .animate_form_list(simulation, selection.form_open_time);
        }

        self.update_buttons(simulation);

        // completion detection

        let all_confirmed = (self.player_selections).iter().all(|selection| {
            selection.erased
                || (!selection.animating_slide
                    && selection.confirm_time != 0
                    && selection.recipe_animation.is_none())
        });

        if all_confirmed {
            self.complete(game_io, resources, simulation);
        }

        self.dark_card_effects(game_io, resources, simulation);

        self.time += 1;
        self.ui.advance_time();
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        if self.completed {
            return;
        }

        let entity_id = simulation.local_player_id;
        let entities = &mut simulation.entities;

        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let selection = &self.player_selections[player.index];
        let selected_item = resolve_selected_item(player, selection);

        // update frame
        if let SelectedItem::Card(i) = selected_item {
            self.ui.update_card_frame(game_io, player, i);
        }

        // draw sprite tree
        self.ui.draw_tree(sprite_queue);
        self.ui.draw_cards_in_hand(game_io, sprite_queue, player);
        self.ui.draw_staged_icons(game_io, sprite_queue, player);

        // draw buttons
        let card_buttons = PlayerOverridables::card_button_slots_for(player);

        if let Some(buttons) = card_buttons {
            for button in buttons.iter().flatten() {
                if let Some(sprite_tree) = simulation.sprite_trees.get_mut(button.sprite_tree_index)
                {
                    sprite_tree.draw(sprite_queue);
                    sprite_queue.set_color_mode(SpriteColorMode::Multiply);
                }
            }
        }

        let special_button = PlayerOverridables::special_button_for(player);

        if let Some(button) = special_button {
            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(button.sprite_tree_index) {
                sprite_tree.draw(sprite_queue);
                sprite_queue.set_color_mode(SpriteColorMode::Multiply);
            }
        }

        // drawing selection
        if let Some(time) = selection.form_open_time {
            // draw form selection
            if self.time > time + CardSelectUi::FORM_LIST_ANIMATION_TIME {
                self.ui
                    .draw_form_list(game_io, sprite_queue, player, selection.form_row);

                // draw cursor
                if selection.form_select_time.is_none() {
                    self.ui.draw_form_cursor(sprite_queue, selection.form_row);
                }
            }
        } else {
            // draw card cursor

            if selection.confirm_time == 0 && !player.card_select_blocked {
                match selected_item {
                    SelectedItem::Card(_) => {
                        self.ui
                            .draw_card_cursor(sprite_queue, selection.col, selection.row)
                    }
                    SelectedItem::Confirm => {
                        self.ui.draw_confirm_cursor(sprite_queue);
                    }
                    SelectedItem::Button(i) => {
                        let find_card_button = |buttons| {
                            CardSelectButton::iter_card_button_slots(buttons)
                                .find(|(_, _, index, _)| *index == i)
                        };

                        if i == CardSelectButton::SPECIAL_SLOT {
                            // special slot
                            self.ui.draw_special_cursor(sprite_queue);
                        } else if let Some(slot_data) = card_buttons.and_then(find_card_button) {
                            // card button
                            let (col, row, _, button) = slot_data;

                            if button.uses_fixed_card_cursor {
                                self.ui.draw_card_cursor(sprite_queue, col, row);
                            } else {
                                let slot_width = button.slot_width as i32;
                                self.ui
                                    .draw_card_button_cursor(sprite_queue, slot_width, col, row);
                            }
                        }
                    }
                    _ => {}
                }
            }

            // draw preview icon

            match selected_item {
                SelectedItem::Card(i) => {
                    let card = &player.deck[i];
                    let preview_point = self.ui.preview_point();

                    card.draw_preview(game_io, sprite_queue, preview_point, 1.0);
                    card.draw_preview_title(game_io, sprite_queue, preview_point);
                }
                SelectedItem::Confirm => {
                    self.ui.draw_confirm_preview(player, sprite_queue);
                }
                SelectedItem::Button(i) => {
                    let button = if i == CardSelectButton::SPECIAL_SLOT {
                        special_button
                    } else {
                        card_buttons.and_then(|b| b.get(i - 1)?.as_ref())
                    };

                    if let Some(button) = button {
                        let index = button.preview_sprite_tree_index;

                        if let Some(sprite_tree) = simulation.sprite_trees.get_mut(index) {
                            sprite_tree.draw(sprite_queue);
                            sprite_queue.set_color_mode(SpriteColorMode::Multiply);
                        }
                    }
                }
                SelectedItem::None => {}
            }
        }

        if player.has_regular_card {
            self.ui.draw_regular_card_frame(sprite_queue);
        }

        if !selection.animating_slide {
            if let Some(animation) = &selection.recipe_animation {
                // animate recipes
                animation.draw(game_io, resources, sprite_queue, player);
            } else if selection.confirm_time != 0 {
                // render text to signal we're waiting on other players
                const MARGIN_TOP: f32 = 38.0;

                let wait_time = self.time - selection.confirm_time;

                if (wait_time / 30) % 2 == 0 {
                    const TEXT: &str = "Waiting...";

                    let mut style = TextStyle::new(game_io, FontName::Thick);
                    style.shadow_color = TEXT_DARK_SHADOW_COLOR;

                    let metrics = style.measure(TEXT);
                    let position = Vec2::new((RESOLUTION_F.x - metrics.size.x) * 0.5, MARGIN_TOP);

                    style.bounds.set_position(position);
                    style.draw(game_io, sprite_queue, TEXT);
                }
            }
        }

        if !simulation.battle_started {
            self.ui.draw_names(game_io, simulation, sprite_queue);
        }

        // draw fade sprite
        if let Some(time) = selection.form_select_time {
            let elapsed = self.time - time;
            let progress =
                inverse_lerp!(FORM_FADE_DELAY, FORM_FADE_DELAY + FORM_FADE_TIME, elapsed);
            let a = crate::ease::quadratic(progress);

            let assets = &globals.assets;

            let mut fade_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
            fade_sprite.set_bounds(Rect::from_corners(Vec2::ZERO, RESOLUTION_F));
            fade_sprite.set_color(Color::WHITE.multiply_alpha(a));
            sprite_queue.draw_sprite(&fade_sprite);
        }
    }
}

impl CardSelectState {
    pub fn new(game_io: &GameIO) -> Self {
        Self {
            ui: CardSelectUi::new(game_io),
            player_selections: Vec::new(),
            time: 0,
            completed: false,
        }
    }

    fn handle_input(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        player_index: usize,
    ) {
        let selection = &mut self.player_selections[player_index];

        if let Some(time) = selection.form_select_time {
            let elapsed = self.time - time;

            match elapsed {
                20 => {
                    // close menu
                    selection.form_open_time = None;

                    // sfx
                    if selection.local {
                        let globals = game_io.resource::<Globals>().unwrap();
                        simulation.play_sound(game_io, &globals.sfx.form_select);
                    }

                    // call form select callback
                    let entities = &mut simulation.entities;

                    if let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) {
                        if let Some(index) = player.staged_items.stored_form_index() {
                            if let Some(callback) = &player.forms[index].select_callback {
                                callback.clone().call(game_io, resources, simulation, ());
                            }
                        }
                    }

                    return;
                }
                40.. => {
                    // unblock input
                    selection.form_select_time = None;
                }
                _ => return,
            }
        }

        if selection.form_open_time.is_some() {
            self.handle_form_input(game_io, resources, simulation, entity_id);
        } else {
            self.handle_card_input(game_io, resources, simulation, entity_id);
        }
    }

    fn handle_form_input(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&mut Player>(entity_id.into()) else {
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let mut pending_sfx = Vec::new();

        let input = &simulation.inputs[player.index];
        let selection = &mut self.player_selections[player.index];

        let prev_row = selection.form_row;
        let available_form_count = player.available_forms().count();

        if input.is_active(Input::Up) {
            if selection.form_row == 0 {
                selection.form_row = available_form_count.max(1) - 1;
            } else {
                selection.form_row -= 1;
            }
        }

        if input.is_active(Input::Down) {
            selection.form_row += 1;

            if selection.form_row >= available_form_count {
                selection.form_row = 0;
            }
        }

        if prev_row != selection.form_row && selection.local {
            // cursor move sfx
            pending_sfx.push(&globals.sfx.cursor_move);
        }

        if input.was_just_pressed(Input::Confirm) {
            // select form
            let form_index = player
                .available_forms()
                .nth(selection.form_row)
                .map(|(index, _)| index);

            if let Some(index) = form_index {
                if let Some(index) = player.staged_items.stored_form_index() {
                    // deselect the form if the player reselected it
                    player.staged_items.drop_form_selection();

                    if let Some(callback) = &player.forms[index].deselect_callback {
                        simulation.pending_callbacks.push(callback.clone());
                    }
                } else {
                    // select new form
                    player.staged_items.stage_form(index, None, None);

                    // select_callback will be called in the middle of the select animation
                    selection.form_select_time = Some(self.time);
                }
            }

            // sfx
            if selection.local {
                pending_sfx.push(&globals.sfx.cursor_select);
            }
        }

        if input.was_just_pressed(Input::Cancel) {
            // close form select
            selection.form_open_time = None;

            // sfx
            if selection.local {
                pending_sfx.push(&globals.sfx.form_select_close);
            }
        }

        if selection.local && !simulation.is_resimulation {
            // dealing with signals
            if input.was_just_pressed(Input::Info) {
                let description = player
                    .available_forms()
                    .nth(selection.form_row)
                    .and_then(|(_, form)| form.description.clone());

                if let Some(description) = description {
                    let event = BattleEvent::Description(description);
                    resources.event_sender.send(event).unwrap();
                }
            }
        }

        for sfx in pending_sfx {
            simulation.play_sound(game_io, sfx);
        }

        simulation.call_pending_callbacks(game_io, resources);
    }

    fn handle_card_input(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&mut Player>(entity_id.into()) else {
            return;
        };

        if player.card_select_blocked {
            return;
        }

        let player_index = player.index;
        let input = &simulation.inputs[player_index];
        let selection = &mut self.player_selections[player_index];

        // see if a script confirmed our selection for us
        if player.staged_items.confirmed() {
            selection.confirm_time = self.time;
            selection.recipe_animation = CardRecipeAnimation::try_new(game_io, resources, player);

            // activate card select close components
            for (_, component) in &simulation.components {
                if component.entity == entity_id
                    && component.lifetime == ComponentLifetime::CardSelectClose
                {
                    let callback = component.update_callback.clone();
                    simulation.pending_callbacks.push(callback);
                }
            }

            simulation.call_pending_callbacks(game_io, resources);

            // ignore input
            return;
        }

        let globals = game_io.resource::<Globals>().unwrap();
        let previous_item = resolve_selected_item(player, selection);

        if input.is_active(Input::End) || previous_item == SelectedItem::None {
            // select Confirm as a safety net
            selection.col = 5;
            selection.row = 0;
        }

        if input.is_active(Input::Up) && selection.row == 0 && player.available_forms().count() > 0
        {
            // open form select
            selection.form_open_time = Some(self.time);

            if selection.local {
                // not using pending_sfx since we're returning right after anyway
                simulation.play_sound(game_io, &globals.sfx.form_select_open);
            }
            return;
        }

        let mut pending_sfx = Vec::new();

        // moving cursor

        if input.is_active(Input::Left) {
            move_card_selection(player, selection, -1, 0);
        }

        if input.is_active(Input::Right) {
            move_card_selection(player, selection, 1, 0);
        }

        if input.is_active(Input::Up) {
            move_card_selection(player, selection, 0, -1);
        }

        if input.is_active(Input::Down) {
            move_card_selection(player, selection, 0, 1);
        }

        let selected_item = resolve_selected_item(player, selection);

        // sfx
        if previous_item != selected_item && selection.local {
            pending_sfx.push(&globals.sfx.cursor_move);
        }

        if input.was_just_pressed(Input::Confirm) {
            match selected_item {
                SelectedItem::Confirm => {
                    selection.confirm_time = self.time;
                    selection.recipe_animation =
                        CardRecipeAnimation::try_new(game_io, resources, player);
                    player.staged_items.set_confirmed(true);

                    // sfx
                    if selection.local {
                        pending_sfx.push(&globals.sfx.card_select_confirm);
                    }

                    // activate card select close components
                    for (_, component) in &simulation.components {
                        if component.entity == entity_id
                            && component.lifetime == ComponentLifetime::CardSelectClose
                        {
                            let callback = component.update_callback.clone();
                            simulation.pending_callbacks.push(callback);
                        }
                    }

                    simulation.call_pending_callbacks(game_io, resources);
                }
                SelectedItem::Card(index) => {
                    if !player.staged_items.has_deck_index(index)
                        && can_player_select(player, index)
                    {
                        let item = StagedItem {
                            data: StagedItemData::Deck(index),
                            undo_callback: None,
                        };

                        player.staged_items.stage_item(item);

                        // sfx
                        if selection.local {
                            pending_sfx.push(&globals.sfx.cursor_select);
                        }
                    } else if selection.local {
                        // error sfx
                        pending_sfx.push(&globals.sfx.cursor_error);
                    }
                }
                SelectedItem::Button(i) => {
                    self.use_button(game_io, resources, simulation, entity_id, i);
                }
                SelectedItem::None => {}
            }
        }

        let input = &simulation.inputs[player_index];

        if input.was_just_pressed(Input::Cancel) {
            self.undo(game_io, resources, simulation, entity_id);
        }

        let input = &simulation.inputs[player_index];
        let selection = &mut self.player_selections[player_index];

        if input.fleeing() || input.disconnected() {
            let entities = &mut simulation.entities;
            let player = entities
                .query_one_mut::<&mut Player>(entity_id.into())
                .unwrap();

            // clear selection
            player.staged_items.clear();

            // confirm
            selection.confirm_time = self.time;
        }

        if selection.local && !simulation.is_resimulation && selection.confirm_time == 0 {
            // dealing with signals
            if input.was_just_pressed(Input::Flee) {
                let event = BattleEvent::RequestFlee;
                resources.event_sender.send(event).unwrap();
            }

            if input.was_just_pressed(Input::Info) {
                match selected_item {
                    SelectedItem::Card(index) => {
                        let entities = &mut simulation.entities;
                        let player = entities.query_one_mut::<&Player>(entity_id.into()).unwrap();

                        let card = &player.deck[index];

                        let event = BattleEvent::DescribeCard(card.package_id.clone());
                        resources.event_sender.send(event).unwrap();
                    }
                    SelectedItem::Button(_) => {
                        let entities = &mut simulation.entities;
                        let player = entities.query_one_mut::<&Player>(entity_id.into()).unwrap();

                        let description = selected_item
                            .button(player)
                            .and_then(|button| button.description.clone());

                        if let Some(description) = description {
                            let event = BattleEvent::Description(description);
                            resources.event_sender.send(event).unwrap();
                        }
                    }
                    _ => {}
                }
            }
        }

        for sfx in pending_sfx {
            simulation.play_sound(game_io, sfx);
        }
    }

    fn use_button(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        button_slot_index: usize,
    ) {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let button = if button_slot_index == CardSelectButton::SPECIAL_SLOT {
            let Some(button) = PlayerOverridables::special_button_for(player) else {
                return;
            };

            button
        } else {
            let Some(card_buttons) = PlayerOverridables::card_button_slots_for(player) else {
                return;
            };

            let Some(Some(button)) = card_buttons.get(button_slot_index - 1) else {
                return;
            };

            button
        };

        let Some(callback) = button.use_callback.clone() else {
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let selection = &mut self.player_selections[player.index];
        let uses_default_audio = button.uses_default_audio;

        let success = callback.call(game_io, resources, simulation, ());

        if success {
            if uses_default_audio && selection.local {
                simulation.play_sound(game_io, &globals.sfx.cursor_select);
            }
        } else if selection.local {
            simulation.play_sound(game_io, &globals.sfx.cursor_error);
        }
    }

    fn undo(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&mut Player>(entity_id.into()) else {
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let local = player.local;
        let mut applied = false;

        if let Some(popped) = player.staged_items.pop() {
            if let StagedItemData::Form((index, ..)) = popped.data {
                if let Some(callback) = &player.forms[index].deselect_callback {
                    callback.clone().call(game_io, resources, simulation, ());
                }

                simulation.play_sound(game_io, &globals.sfx.form_deactivate);
            }

            if let Some(callback) = popped.undo_callback {
                callback.call(game_io, resources, simulation, ());
            }

            applied = true;
        }

        // sfx
        if local {
            if applied {
                simulation.play_sound(game_io, &globals.sfx.cursor_cancel);
            } else {
                simulation.play_sound(game_io, &globals.sfx.cursor_error);
            }
        }
    }

    fn complete(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let entities = &mut simulation.entities;
        for (_, (player, character)) in entities.query_mut::<(&mut Player, &mut Character)>() {
            if let Some(index) = player.staged_items.stored_form_index() {
                // change form
                player.active_form = Some(index);
                player.forms[index].activated = false;
            }

            if player.staged_items.visible_count() > 0 {
                // only clear if there's no visible changes, we keep cards from the previous selection in that case
                character.cards.clear();
                character.next_card_mutation = Some(0);
            }

            // load cards in reverse as we'll pop them off in battle state (first item must be last)
            let namespace = player.namespace();
            character.cards.extend(
                player
                    .staged_items
                    .resolve_card_properties(game_io, resources, namespace, &player.deck)
                    .rev(),
            );

            // remove the cards from the deck
            // must sort + loop in reverse to prevent issues from shifting indices
            let mut deck_indices: Vec<_> = player.staged_items.deck_card_indices().collect();
            deck_indices.sort();

            for i in deck_indices.iter().rev() {
                if player.deck.get(*i).is_some() {
                    player.deck.remove(*i);
                }
            }

            // unmark player as having regular card
            if player.has_regular_card && player.staged_items.has_deck_index(0) {
                player.has_regular_card = false;
            }

            player.staged_items.clear();
        }

        Character::mutate_cards(game_io, resources, simulation);

        simulation.update_components(game_io, resources, ComponentLifetime::CardSelectComplete);

        self.completed = true;
    }

    fn update_buttons(&mut self, simulation: &mut BattleSimulation) {
        let entities = &mut simulation.entities;
        let sprite_trees = &mut simulation.sprite_trees;
        let animators = &mut simulation.animators;
        let pending_callbacks = &mut simulation.pending_callbacks;

        let card_start_point = self.ui.card_start_point();

        let mut preview_position = self.ui.preview_point();
        preview_position.x -= CARD_PREVIEW_SIZE.x * 0.5;

        let mut update_button = move |selected_item, i, position, button: &mut CardSelectButton| {
            button.animate_sprite(sprite_trees, animators, pending_callbacks, position);

            if selected_item == SelectedItem::Button(i) {
                button.animate_preview_sprite(
                    sprite_trees,
                    animators,
                    pending_callbacks,
                    preview_position,
                );
            }
        };

        // we must update buttons for every player to keep the game in sync
        for (_, player) in entities.query_mut::<&mut Player>() {
            let selection = &mut self.player_selections[player.index];
            let selected_item = resolve_selected_item(player, selection);

            if let Some(buttons) = PlayerOverridables::card_button_slots_mut_for(player) {
                for (col, row, i, button) in CardSelectButton::iter_card_button_slots_mut(buttons) {
                    update_button(
                        selected_item,
                        i,
                        CardSelectUi::calculate_icon_position(card_start_point, col, row),
                        button,
                    );
                }
            }

            if let Some(button) = PlayerOverridables::special_button_mut_for(player) {
                update_button(
                    selected_item,
                    CardSelectButton::SPECIAL_SLOT,
                    self.ui.special_button_point(),
                    button,
                );

                selection.has_special_button = true;
            }
        }
    }

    fn dark_card_check(&mut self, simulation: &mut BattleSimulation, game_io: &GameIO) {
        let player_id_vec = simulation
            .entities
            .query_mut::<&Player>()
            .into_iter()
            .map(|(id, _player)| (id.into()))
            .collect::<Vec<EntityId>>();

        let entities = &mut simulation.entities;

        for id in player_id_vec {
            let player = entities.query_one_mut::<&mut Player>(id.into()).unwrap();

            let card_view_size = player.deck.len().min(player.hand_size());

            for index in 0..card_view_size {
                let card = &player.deck[index];
                let globals = game_io.resource::<Globals>().unwrap();

                let card_packages = &globals.card_packages;
                let namespace = player.namespace();

                let is_dark = card_packages
                    .package_or_fallback(namespace, &card.package_id)
                    .is_some_and(|package| package.card_properties.card_class == CardClass::Dark);

                if !is_dark {
                    continue;
                };

                // update selection
                let selection = &mut self.player_selections[player.index];

                let x = (index % CARD_COLS) as i32;
                let y = (index / CARD_COLS) as i32;

                let x_difference = x - selection.col;
                let y_difference = y - selection.row;

                move_card_selection(player, selection, x_difference, y_difference);

                break;
            }
        }
    }

    fn dark_card_effects(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let entities = &mut simulation.entities;

        let Ok(player) = entities.query_one_mut::<&mut Player>(simulation.local_player_id.into())
        else {
            return;
        };

        let selection = &mut self.player_selections[player.index];
        let SelectedItem::Card(deck_index) = resolve_selected_item(player, selection) else {
            return;
        };

        let card = &player.deck[deck_index];

        let globals = game_io.resource::<Globals>().unwrap();

        let card_packages = &globals.card_packages;
        let namespace = player.namespace();

        let is_dark = card_packages
            .package_or_fallback(namespace, &card.package_id)
            .is_some_and(|package| package.card_properties.card_class == CardClass::Dark);

        if is_dark {
            let mut pending_sfx = Vec::new();

            let globals = game_io.resource::<Globals>().unwrap();

            pending_sfx.push(&globals.sfx.dark_card);

            resources.ui_fade_color.set(Color::new(0.0, 0.0, 0.0, 0.5));

            for sfx in pending_sfx {
                let audio = &globals.audio;
                audio.play_sound_with_behavior(sfx, AudioBehavior::NoOverlap);
            }
        }
    }
}

fn resolve_selected_item(player: &Player, selection: &Selection) -> SelectedItem {
    let col = selection.col as usize;
    let row = selection.row as usize;

    if row == 0 && col == CARD_SELECT_COLS - 1 {
        return SelectedItem::Confirm;
    }

    if row == 1 && col == CARD_SELECT_COLS - 1 {
        return if selection.has_special_button {
            SelectedItem::Button(CardSelectButton::SPECIAL_SLOT)
        } else {
            SelectedItem::None
        };
    }

    if let Some(buttons) = PlayerOverridables::card_button_slots_for(player) {
        for (c, r, i, _) in CardSelectButton::iter_card_button_slots(buttons) {
            if row == r as usize && col >= c as usize {
                return SelectedItem::Button(i);
            }
        }
    }

    let card_view_size = player.deck.len().min(player.hand_size());
    let card_index = row * CARD_COLS + col;

    if card_index < card_view_size {
        return SelectedItem::Card(card_index);
    }

    // todo: Card

    SelectedItem::None
}

fn can_player_select(player: &Player, index: usize) -> bool {
    if player.staged_items.visible_count() >= 5 {
        return false;
    }

    let restriction = CardSelectRestriction::resolve(player);
    let card = &player.deck[index];

    restriction.allows_card(card)
}

fn move_card_selection(player: &Player, selection: &mut Selection, col_diff: i32, row_diff: i32) {
    let previous_item = resolve_selected_item(player, selection);

    // move row
    let previous_row = selection.row;
    selection.row += row_diff;

    // wrap row
    if selection.row == -1 {
        selection.row = CARD_SELECT_ROWS as i32 - 1;
    } else {
        selection.row %= CARD_SELECT_ROWS as i32;
    }

    if col_diff == 0 {
        if resolve_selected_item(player, selection) == SelectedItem::None {
            selection.row = previous_row;
        }

        return;
    }

    // scan row to change behavior
    let mut test_other_rows = true;

    if selection.col != CARD_SELECT_COLS as i32 - 1 {
        let original_col = selection.col;

        let row_scan_range = if col_diff > 0 {
            0..CARD_SELECT_CARD_COLS as i32
        } else {
            0..CARD_SELECT_COLS as i32
        };

        loop {
            selection.col += col_diff;

            if !row_scan_range.contains(&selection.col) {
                break;
            }

            let item = resolve_selected_item(player, selection);

            if item != previous_item && item != SelectedItem::None {
                test_other_rows = false;
                break;
            }
        }

        selection.col = original_col;
    }

    // move col
    for _ in 0..CARD_SELECT_COLS {
        selection.col += col_diff;

        // wrap
        if selection.col == -1 {
            selection.col = CARD_SELECT_COLS as i32 - 1;
        } else {
            selection.col %= CARD_SELECT_COLS as i32;
        }

        let item = resolve_selected_item(player, selection);

        if item == SelectedItem::None {
            if test_other_rows {
                let previous_row = selection.row;

                for row in 0..CARD_SELECT_ROWS as i32 {
                    selection.row = row;

                    if selection.row == previous_row {
                        continue;
                    }

                    if resolve_selected_item(player, selection) != SelectedItem::None {
                        return;
                    }
                }

                // restore row
                selection.row = previous_row;
            }

            // continue tests
            continue;
        }

        if item == previous_item {
            // selection takes up multiple slots
            continue;
        }

        return;
    }
}
