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
    card_button_width: usize,
    has_special_button: bool,
    form_select_time: Option<FrameTime>,
    form_open_time: Option<FrameTime>,
    confirm_time: FrameTime,
    animating_slide: bool,
    erased: bool,
    local: bool,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum SelectedItem {
    Card(usize),
    CardButton,
    SpecialButton,
    Confirm,
    None,
}

impl SelectedItem {
    fn button(self, player: &Player) -> Option<&CardSelectButton> {
        PlayerOverridables::flat_map_for(player, move |overridables| match self {
            SelectedItem::CardButton => overridables.card_button.as_deref(),
            SelectedItem::SpecialButton => overridables.special_button.as_deref(),
            _ => unreachable!(),
        })
        .next()
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

            // reset staged items confirmation
            for (_, player) in simulation.entities.query_mut::<&mut Player>() {
                player.staged_items.set_confirmed(false);
            }

            // sfx
            let globals = game_io.resource::<Globals>().unwrap();
            simulation.play_sound(game_io, &globals.sfx.card_select_open);

            simulation.update_components(game_io, resources, ComponentLifetime::CardSelectOpen);
        }

        CardSelectButton::update_all_from_staged_items(game_io, resources, simulation);

        for selection in &mut self.player_selections {
            // assume erased until we hit the next loop
            selection.erased = true;
        }

        // we'll only see our own selection
        // but we must simulate every player's input to stay in sync
        let id_index_local_tuples = simulation
            .entities
            .query_mut::<&Player>()
            .into_iter()
            .map(|(id, player)| (id.into(), player.index, player.local))
            .collect::<Vec<(EntityId, usize, bool)>>();

        for (entity_id, player_index, local) in id_index_local_tuples {
            if player_index >= self.player_selections.len() {
                self.player_selections
                    .resize_with(player_index + 1, Selection::default);

                // initialize selection
                let selection = &mut self.player_selections[player_index];
                selection.local = local;
                selection.animating_slide = true;
            }

            let selection = &mut self.player_selections[player_index];

            // unmark as erased
            selection.erased = false;

            if selection.animating_slide || selection.confirm_time != 0 {
                // if we're animating the ui or already confirmed we should skip this input
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
            selection.erased || (!selection.animating_slide && selection.confirm_time != 0)
        });

        if all_confirmed {
            self.complete(game_io, resources, simulation);
        }

        self.time += 1;
        self.ui.advance_time();
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO,
        _resources: &SharedBattleResources,
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
        let card_button = PlayerOverridables::flat_map_for(player, |overridables| {
            overridables.card_button.as_ref()
        })
        .next();

        if let Some(button) = card_button {
            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(button.sprite_tree_index) {
                sprite_tree.draw(sprite_queue);
                sprite_queue.set_color_mode(SpriteColorMode::Multiply);
            }
        }

        let special_button = PlayerOverridables::flat_map_for(player, |card_button| {
            card_button.special_button.as_ref()
        })
        .next();

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
                    SelectedItem::SpecialButton => {
                        self.ui.draw_special_cursor(sprite_queue);
                    }
                    SelectedItem::CardButton => {
                        if card_button.is_some_and(|button| button.uses_fixed_card_cursor) {
                            let col = (CARD_SELECT_CARD_COLS - 1) as i32;
                            let row = (CARD_SELECT_ROWS - 1) as i32;
                            self.ui.draw_card_cursor(sprite_queue, col, row);
                        } else {
                            let slot_width = selection.card_button_width;
                            self.ui.draw_card_button_cursor(sprite_queue, slot_width);
                        }
                    }
                    _ => {}
                }
            }

            if player.has_regular_card {
                self.ui.draw_regular_card_frame(sprite_queue);
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
                SelectedItem::CardButton | SelectedItem::SpecialButton => {
                    let button = match selected_item {
                        SelectedItem::CardButton => card_button,
                        SelectedItem::SpecialButton => special_button,
                        _ => unreachable!(),
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

        if !simulation.battle_started {
            self.ui.draw_names(game_io, simulation, sprite_queue);
        }

        if !selection.animating_slide && selection.confirm_time != 0 {
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
                    let globals = game_io.resource::<Globals>().unwrap();
                    simulation.play_sound(game_io, &globals.sfx.transform_select);

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
                if player.staged_items.stored_form_index() == Some(index) {
                    // deselect the form if the player reselected it
                    player.staged_items.drop_form_selection();
                } else {
                    // select new form
                    player.staged_items.stage_form(index, None, None);
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

        let globals = game_io.resource::<Globals>().unwrap();
        let mut pending_sfx = Vec::new();

        let player_index = player.index;
        let input = &simulation.inputs[player_index];
        let selection = &mut self.player_selections[player_index];

        // see if a script confirmed our selection for us
        if player.staged_items.confirmed() {
            selection.confirm_time = self.time;

            // ignore input
            return;
        }

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

            pending_sfx.push(&globals.sfx.form_select_open);
            return;
        }

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
                    player.staged_items.set_confirmed(true);

                    // sfx
                    if selection.local {
                        pending_sfx.push(&globals.sfx.card_select_confirm);
                    }
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
                SelectedItem::CardButton | SelectedItem::SpecialButton => {
                    self.use_button(game_io, resources, simulation, entity_id, selected_item);
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
                    SelectedItem::CardButton | SelectedItem::SpecialButton => {
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
        selected_item: SelectedItem,
    ) {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let card_button = selected_item.button(player);

        let Some(button) = card_button else {
            return;
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
                player.deck.remove(*i);
            }

            // unmark player as having regular card
            if player.has_regular_card && player.staged_items.has_deck_index(0) {
                player.has_regular_card = false;
            }

            player.staged_items.clear();
        }

        Character::mutate_cards(game_io, resources, simulation);

        simulation.update_components(game_io, resources, ComponentLifetime::CardSelectClose);

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

        // we must update buttons for every player to keep the game in sync
        for (_, player) in entities.query_mut::<&mut Player>() {
            let selection = &mut self.player_selections[player.index];
            let selected_item = resolve_selected_item(player, selection);

            let card_button = PlayerOverridables::flat_map_mut_for(player, |overridables| {
                overridables.card_button.as_mut()
            })
            .next();

            if let Some(button) = card_button {
                // update slot width
                let button_width = button.slot_width.min(CARD_COLS);
                selection.card_button_width = button_width;

                // animate
                let position = CardSelectUi::calculate_icon_position(
                    card_start_point,
                    CARD_COLS.saturating_sub(button_width) as i32,
                    CARD_SELECT_ROWS as i32 - 1,
                );

                button.animate_sprite(sprite_trees, animators, pending_callbacks, position);

                // animate preview sprite
                if selected_item == SelectedItem::CardButton {
                    button.animate_preview_sprite(
                        sprite_trees,
                        animators,
                        pending_callbacks,
                        preview_position,
                    );
                }
            }

            let special_button = PlayerOverridables::flat_map_mut_for(player, |overridables| {
                overridables.special_button.as_mut()
            })
            .next();

            selection.has_special_button = special_button.is_some();

            if let Some(button) = special_button {
                // animate
                let position = self.ui.special_button_point();
                button.animate_sprite(sprite_trees, animators, pending_callbacks, position);

                // animate preview sprite
                if selected_item == SelectedItem::SpecialButton {
                    button.animate_preview_sprite(
                        sprite_trees,
                        animators,
                        pending_callbacks,
                        preview_position,
                    );
                }
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
            SelectedItem::SpecialButton
        } else {
            SelectedItem::Confirm
        };
    }

    let card_button_range = (CARD_COLS - selection.card_button_width)..(CARD_COLS + 1);

    if row == 1 && card_button_range.contains(&col) {
        return SelectedItem::CardButton;
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
    let previous_selection = resolve_selected_item(player, selection);

    // 6 attempts, we should've looped back around by then
    for _ in 0..6 {
        selection.col += col_diff;
        selection.row += row_diff;

        if selection.col == -1 {
            selection.col = CARD_SELECT_COLS as i32 - 1;
        }

        if selection.row == -1 {
            selection.row = CARD_SELECT_ROWS as i32 - 1;
        }

        selection.col %= CARD_SELECT_COLS as i32;
        selection.row %= CARD_SELECT_ROWS as i32;

        let new_selection = resolve_selected_item(player, selection);

        if new_selection == SelectedItem::None {
            continue;
        }

        if new_selection == previous_selection {
            // selection takes up multiple slots
            continue;
        }

        return;
    }
}
