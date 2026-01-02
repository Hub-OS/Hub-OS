use super::State;
use crate::battle::*;
use crate::bindable::*;
use crate::ease::inverse_lerp;
use crate::lua_api::CardDamageResolver;
use crate::packages::PackageNamespace;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::scenes::BattleEvent;
use crate::transitions::transparent_flash_color;
use framework::prelude::*;

const FORM_FADE_DELAY: FrameTime = 10;
const FORM_FADE_TIME: FrameTime = 20;
const CARD_COLS: usize = CARD_SELECT_CARD_COLS;

#[derive(Clone)]
pub struct CardSelectSelection {
    pub col: i32,
    pub row: i32,
    pub form_row: usize,
    pub hovered_form: Option<usize>,
    pub has_special_button: bool,
    pub form_select_time: Option<FrameTime>,
    pub form_open_time: Option<FrameTime>,
    pub confirm_time: FrameTime,
    pub animating_slide: bool,
    pub recipe_animation: Option<CardRecipeAnimation>,
    pub visible: bool,
    pub erased: bool,
    pub local: bool,
    pub resolved_card_damage: i32,
}

impl CardSelectSelection {
    fn default() -> Self {
        Self {
            col: 0,
            row: 0,
            form_row: 0,
            hovered_form: None,
            has_special_button: false,
            form_select_time: None,
            form_open_time: None,
            confirm_time: 0,
            animating_slide: false,
            recipe_animation: None,
            visible: true,
            erased: false,
            local: false,
            resolved_card_damage: Default::default(),
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
        let SelectedItem::Button(index) = self else {
            return None;
        };

        if index == CardSelectButton::SPECIAL_SLOT {
            return PlayerOverridables::special_button_for(player);
        }

        let index = index - 1;
        PlayerOverridables::card_button_slots_for(player)
            .and_then(|buttons| buttons.get(index)?.as_ref())
    }
}

#[derive(Clone)]
pub struct CardSelectState {
    ui: CardSelectUi,
    player_selections: Vec<CardSelectSelection>,
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

            type Query<'a> = (&'a Entity, &'a Player, &'a mut PlayerHand);

            // reset staged items confirmation and initialize selection data
            for (_, (_, player, hand)) in simulation.entities.query_mut::<Query>() {
                hand.staged_items.set_confirmed(false);

                if player.index >= self.player_selections.len() {
                    self.player_selections
                        .resize_with(player.index + 1, CardSelectSelection::default);
                }

                // initialize selection
                let selection = &mut self.player_selections[player.index];
                selection.local = player.index == simulation.local_player_index;
                selection.animating_slide = true;
            }

            simulation.update_components(game_io, resources, ComponentLifetime::CardSelectOpen);

            // handle frame 0 scripted confirmation
            let mut pre_confirmed = false;
            for (id, (entity, player, hand)) in simulation.entities.query_mut::<Query>() {
                if !entity.deleted && !hand.staged_items.confirmed() {
                    continue;
                }

                if player.index == simulation.local_player_index {
                    pre_confirmed = true;
                }

                // update data
                let selection = &mut self.player_selections[player.index];
                selection.confirm_time = -CardSelectUi::SLIDE_DURATION;
                selection.recipe_animation =
                    CardRecipeAnimation::try_new(game_io, resources, player.namespace(), hand);

                for (_, component) in &simulation.components {
                    if component.entity == id.into()
                        && component.lifetime == ComponentLifetime::CardSelectClose
                    {
                        let callback = component.update_callback.clone();
                        simulation.pending_callbacks.push(callback);
                    }
                }
            }

            simulation.call_pending_callbacks(game_io, resources);

            // sfx
            if !pre_confirmed {
                let globals = Globals::from_resources(game_io);
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

            let scale = simulation.field.best_fitting_scale();

            if scale.x < 1.0 {
                // special camera handling for large fields
                simulation.camera.zoom(scale, BATTLE_ZOOM_MOTION);

                if selection.confirm_time == 0 {
                    simulation
                        .camera
                        .slide(BATTLE_CARD_SELECT_CAMERA_OFFSET, BATTLE_ZOOM_MOTION);
                } else {
                    simulation
                        .camera
                        .slide(BATTLE_CAMERA_OFFSET, BATTLE_ZOOM_MOTION);
                }
            } else {
                // regular camera handling
                let progress = self.ui.slide_progress(selection.confirm_time);

                let camera_start = BATTLE_CAMERA_OFFSET;
                let camera_end = BATTLE_CARD_SELECT_CAMERA_OFFSET;
                let camera_position = camera_start.lerp(camera_end, progress).floor();
                simulation.camera.snap(camera_position);
            }

            self.ui.animate_slide(simulation, selection);
            self.ui.animate_form_list(simulation, selection);
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

        let Ok((player, hand)) = entities.query_one_mut::<(&Player, &PlayerHand)>(entity_id.into())
        else {
            return;
        };

        let selection = &self.player_selections[player.index];

        let fully_closed = selection.confirm_time > 0 && !selection.animating_slide;

        if !selection.visible && !fully_closed {
            return;
        }

        let selected_item = resolve_selected_item(player, hand, selection);

        // update frame
        if let SelectedItem::Card(i) = selected_item {
            self.ui.update_card_frame(game_io, hand, i);
        } else {
            self.ui.reset_card_frame();
        }

        // draw sprite tree
        self.ui.draw_tree(sprite_queue);
        self.ui
            .draw_cards_in_hand(game_io, sprite_queue, player, hand);
        self.ui.draw_staged_icons(game_io, sprite_queue, hand);

        // draw buttons
        let card_buttons = PlayerOverridables::card_button_slots_for(player);

        if let Some(buttons) = card_buttons {
            for button in buttons.iter().flatten() {
                if let Some(sprite_tree) = simulation.sprite_trees.get_mut(button.sprite_tree_index)
                {
                    sprite_tree.draw(sprite_queue);
                }
            }
        }

        let special_button = PlayerOverridables::special_button_for(player);

        if let Some(button) = special_button
            && let Some(sprite_tree) = simulation.sprite_trees.get_mut(button.sprite_tree_index)
        {
            sprite_tree.draw(sprite_queue);
        }

        // drawing selection
        if let Some(time) = selection.form_open_time {
            // draw form selection
            if self.time > time + CardSelectUi::FORM_LIST_ANIMATION_TIME {
                let selected_row = if selection.form_select_time.is_none() {
                    Some(selection.form_row)
                } else {
                    None
                };

                self.ui
                    .draw_form_list(game_io, sprite_queue, player, hand, selected_row);

                // draw cursor
                if selection.form_select_time.is_none() {
                    self.ui.draw_form_cursor(sprite_queue, selection.form_row);
                }
            }
        } else {
            // draw card cursor

            if selection.confirm_time == 0 && !hand.card_select_blocked {
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
                    let (card, namespace) = &hand.deck[i];
                    let preview_point = self.ui.preview_point();

                    let mut preview = CardPreview::new(card, preview_point, 1.0);
                    preview.namespace = *namespace;
                    preview.damage_override = Some(selection.resolved_card_damage);
                    preview.draw(game_io, sprite_queue);
                    preview.draw_title(game_io, sprite_queue);
                }
                SelectedItem::Confirm => {
                    self.ui.draw_confirm_preview(hand, sprite_queue);
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
                        }
                    }
                }
                SelectedItem::None => {}
            }
        }

        if hand.has_regular_card {
            self.ui.draw_regular_card_frame(sprite_queue);
        }

        if !selection.animating_slide {
            if let Some(animation) = &selection.recipe_animation {
                // animate recipes
                animation.draw(game_io, resources, sprite_queue, player.namespace());
            } else if selection.confirm_time == 0 {
                // render indicators while the cust is open
                if Some(simulation.statistics.turns) == resources.config.borrow().turn_limit {
                    self.ui.draw_final_turn_indicator(sprite_queue);
                }
            } else {
                // render text to signal we're waiting on other players
                const MARGIN_TOP: f32 = 38.0;

                let wait_time = self.time - selection.confirm_time;

                if (wait_time / 30) % 2 == 0 {
                    let globals = Globals::from_resources(game_io);
                    let text = globals.translate("battle-waiting");

                    let mut style = TextStyle::new(game_io, FontName::Thick);
                    style.shadow_color = TEXT_DARK_SHADOW_COLOR;

                    let metrics = style.measure(&text);
                    let position = Vec2::new((RESOLUTION_F.x - metrics.size.x) * 0.5, MARGIN_TOP);

                    style.bounds.set_position(position);
                    style.draw(game_io, sprite_queue, &text);
                }
            }
        }

        if simulation.progress < BattleProgress::BattleStarted && selection.confirm_time == 0 {
            self.ui.draw_names(game_io, simulation, sprite_queue);
        }

        // update the fade sprite color
        if let Some(time) = selection.form_select_time {
            let elapsed = self.time - time;
            let progress =
                inverse_lerp!(FORM_FADE_DELAY, FORM_FADE_DELAY + FORM_FADE_TIME, elapsed);
            let a = crate::ease::quadratic(progress);

            let prev_color = resources.ui_fade_color.get();
            let target_color = transparent_flash_color(game_io);
            let color = Color::lerp(prev_color, target_color, a);
            resources.ui_fade_color.set(color);
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
                    let entities = &mut simulation.entities;
                    let Ok((player, hand)) =
                        entities.query_one_mut::<(&Player, &PlayerHand)>(entity_id.into())
                    else {
                        return;
                    };

                    let Some(index) = hand.staged_items.stored_form_index() else {
                        return;
                    };

                    let form = &player.forms[index];

                    // close menu
                    if form.close_on_select {
                        selection.form_open_time = None;
                    }

                    // call form select callback
                    if let Some(callback) = &form.select_callback {
                        callback.clone().call(game_io, resources, simulation, ());
                    }

                    // sfx
                    if selection.local {
                        let globals = Globals::from_resources(game_io);
                        simulation.play_sound(game_io, &globals.sfx.form_select);
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

        let entities = &mut simulation.entities;
        let Ok((entity, player, hand)) =
            entities.query_one_mut::<(&Entity, &Player, &mut PlayerHand)>(entity_id.into())
        else {
            return;
        };

        if hand.card_select_blocked {
            return;
        }

        let input = &simulation.inputs[player_index];

        // test for deletion
        if entity.deleted
            || input.fleeing()
            || input.disconnected()
            || simulation.progress >= BattleProgress::BattleEnded
        {
            // confirm
            selection.confirm_time = self.time;
            hand.staged_items.set_confirmed(true);

            // clear selection
            while let Some(popped) = hand.staged_items.pop() {
                if let StagedItemData::Form((index, ..)) = popped.data
                    && let Some(callback) = &player.forms[index].deselect_callback
                {
                    simulation.pending_callbacks.push(callback.clone());
                }

                if let Some(callback) = popped.undo_callback {
                    simulation.pending_callbacks.push(callback);
                }
            }

            // fall through for the script confirmation test
        }

        // see if a script confirmed our selection for us
        if hand.staged_items.confirmed() {
            selection.confirm_time = self.time;
            selection.recipe_animation =
                CardRecipeAnimation::try_new(game_io, resources, player.namespace(), hand);

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

        // toggle ui visibility
        if input.was_just_pressed(Input::Option2) {
            selection.visible = !selection.visible;

            if selection.local {
                let globals = Globals::from_resources(game_io);
                simulation.play_sound(game_io, &globals.sfx.card_select_toggle);
            }
        }

        if !selection.visible {
            return;
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
        let Ok((player, hand)) =
            entities.query_one_mut::<(&Player, &mut PlayerHand)>(entity_id.into())
        else {
            return;
        };

        let globals = Globals::from_resources(game_io);
        let mut pending_sfx = Vec::new();

        let selection = &mut self.player_selections[player.index];

        // try to stick to the previous selection if the form list changed
        if let Some(hovered_index) = selection.hovered_form {
            let initial_hover = player
                .available_forms()
                .nth(selection.form_row)
                .map(|(index, _)| index);

            if initial_hover != selection.hovered_form {
                selection.form_row = player
                    .available_forms()
                    .position(|(index, _)| index == hovered_index)
                    .unwrap_or_default();
            }
        }

        // handle input
        let input = &simulation.inputs[player.index];
        let available_form_count = player.available_forms().count();

        let prev_row = selection.form_row;

        if input.pulsed(Input::Up) {
            if selection.form_row == 0 {
                selection.form_row = available_form_count.max(1) - 1;
            } else {
                selection.form_row -= 1;
            }
        }

        if input.pulsed(Input::Down) {
            selection.form_row += 1;

            if selection.form_row >= available_form_count {
                selection.form_row = 0;
            }
        }

        if prev_row != selection.form_row && selection.local {
            // cursor move sfx
            pending_sfx.push(&globals.sfx.cursor_move);
        }

        selection.hovered_form = player
            .available_forms()
            .nth(selection.form_row)
            .map(|(index, _)| index);

        // select form
        if input.was_just_pressed(Input::Confirm)
            && let Some(index) = selection.hovered_form
        {
            let prev_index = hand.staged_items.stored_form_index();

            // deselect the previous form
            if let Some((prev_index, undo_callback)) = hand.staged_items.drop_form_selection() {
                if let Some(callback) = &player.forms[prev_index].deselect_callback {
                    simulation.pending_callbacks.push(callback.clone());
                }

                if let Some(callback) = undo_callback {
                    simulation.pending_callbacks.push(callback);
                }
            }

            if prev_index != selection.hovered_form {
                // select new form
                hand.staged_items.stage_form(index, None, None);

                let form = &player.forms[index];

                if form.transition_on_select {
                    // select_callback will be called in the middle of the select animation
                    selection.form_select_time = Some(self.time);
                } else {
                    if form.close_on_select {
                        selection.form_open_time = None;
                    }

                    if let Some(callback) = &form.select_callback {
                        // select immediately
                        simulation.pending_callbacks.push(callback.clone());
                    }
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
        let Ok((player, hand)) =
            entities.query_one_mut::<(&Player, &mut PlayerHand)>(entity_id.into())
        else {
            return;
        };

        let player_index = player.index;
        let input = &simulation.inputs[player_index];
        let selection = &mut self.player_selections[player_index];

        let globals = Globals::from_resources(game_io);
        let previous_item = resolve_selected_item(player, hand, selection);

        if input.pulsed(Input::End) || previous_item == SelectedItem::None {
            // select Confirm as a safety net
            selection.col = 5;
            selection.row = 0;
        }

        if input.pulsed(Input::Up) && selection.row == 0 && player.available_forms().count() > 0 {
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

        if input.pulsed(Input::Left) {
            move_card_selection(player, hand, selection, -1, 0);
        }

        if input.pulsed(Input::Right) {
            move_card_selection(player, hand, selection, 1, 0);
        }

        if input.pulsed(Input::Up) {
            move_card_selection(player, hand, selection, 0, -1);
        }

        if input.pulsed(Input::Down) {
            move_card_selection(player, hand, selection, 0, 1);
        }

        let selected_item = resolve_selected_item(player, hand, selection);

        // sfx
        if previous_item != selected_item && selection.local {
            pending_sfx.push((&globals.sfx.cursor_move, AudioBehavior::Default));
        }

        if input.was_just_pressed(Input::Confirm) {
            match selected_item {
                SelectedItem::Confirm => {
                    selection.confirm_time = self.time;
                    selection.recipe_animation =
                        CardRecipeAnimation::try_new(game_io, resources, player.namespace(), hand);
                    hand.staged_items.set_confirmed(true);

                    // sfx
                    if selection.local {
                        pending_sfx
                            .push((&globals.sfx.card_select_confirm, AudioBehavior::Default));
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
                    if !hand.staged_items.has_deck_index(index) && can_player_select(hand, index) {
                        let item = StagedItem {
                            data: StagedItemData::Deck(index),
                            undo_callback: None,
                        };

                        hand.staged_items.stage_item(item);

                        // sfx
                        if selection.local {
                            pending_sfx.push((&globals.sfx.cursor_select, AudioBehavior::Default));
                        }
                    } else if selection.local {
                        // error sfx
                        pending_sfx.push((&globals.sfx.cursor_error, AudioBehavior::NoOverlap));
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

        // resolve dynamic damage
        let selection = &mut self.player_selections[player_index];

        if let SelectedItem::Card(deck_index) = selected_item {
            selection.resolved_card_damage = Player::resolve_dynamic_damage(
                game_io, resources, simulation, entity_id, deck_index,
            );
        }

        // dealing with signals
        let input = &simulation.inputs[player_index];
        let selection = &mut self.player_selections[player_index];

        if selection.local && !simulation.is_resimulation && selection.confirm_time == 0 {
            if input.was_just_pressed(Input::Flee) {
                let event = BattleEvent::RequestFlee;
                resources.event_sender.send(event).unwrap();
            }

            if input.was_just_pressed(Input::Info) {
                match selected_item {
                    SelectedItem::Card(index) => {
                        let entities = &mut simulation.entities;
                        let hand = entities
                            .query_one_mut::<&PlayerHand>(entity_id.into())
                            .unwrap();

                        let (card, namespace) = &hand.deck[index];

                        let event = BattleEvent::DescribeCard(*namespace, card.package_id.clone());
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

        for (sfx, behavior) in pending_sfx {
            simulation.play_sound_with_behavior(game_io, sfx, behavior);
        }

        if selection.local {
            self.dark_card_effects(game_io, resources, simulation);
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

        let globals = Globals::from_resources(game_io);
        let selection = &mut self.player_selections[player.index];
        let uses_default_audio = button.uses_default_audio;

        let success = callback.call(game_io, resources, simulation, ());

        if success {
            if uses_default_audio && selection.local {
                simulation.play_sound(game_io, &globals.sfx.cursor_select);
            }
        } else if selection.local {
            simulation.play_sound_with_behavior(
                game_io,
                &globals.sfx.cursor_error,
                AudioBehavior::NoOverlap,
            );
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
        let Ok((player, hand)) =
            entities.query_one_mut::<(&Player, &mut PlayerHand)>(entity_id.into())
        else {
            return;
        };

        let globals = Globals::from_resources(game_io);
        let local = player.index == simulation.local_player_index;
        let mut applied = false;

        if let Some(popped) = hand.staged_items.pop() {
            if let StagedItemData::Form((index, ..)) = popped.data {
                if let Some(callback) = &player.forms[index].deselect_callback {
                    callback.clone().call(game_io, resources, simulation, ());
                }

                if local {
                    simulation.play_sound(game_io, &globals.sfx.form_deactivate);
                }
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
                simulation.play_sound_with_behavior(
                    game_io,
                    &globals.sfx.cursor_error,
                    AudioBehavior::NoOverlap,
                );
            }
        }
    }

    fn complete(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut player_ids = Vec::new();

        let entities = &mut simulation.entities;
        for (id, (player, hand, character)) in
            entities.query_mut::<(&mut Player, &mut PlayerHand, &mut Character)>()
        {
            if let Some(index) = hand.staged_items.stored_form_index() {
                // change form
                player.active_form = Some(index);
                player.form_boost_order = player.augments.len();
                player.forms[index].activated = false;
            }

            if hand.staged_items.visible_count() > 0 {
                // only clear if there's no visible changes, we keep cards from the previous selection in that case
                character.cards.clear();
                character.next_card_mutation = Some(0);
            }

            // load cards in reverse as we'll pop them off in battle state (first item must be last)
            character.cards.extend(
                hand.staged_items
                    .resolve_card_properties(game_io, resources, &hand.deck)
                    .rev(),
            );

            // remove the cards from the deck
            // must sort + loop in reverse to prevent issues from shifting indices
            let mut deck_indices: Vec<_> = hand.staged_items.deck_card_indices().collect();
            deck_indices.sort();

            for i in deck_indices.iter().rev() {
                if hand.deck.get(*i).is_some() {
                    hand.deck.remove(*i);
                }
            }

            // unmark player as having regular card
            if hand.has_regular_card && hand.staged_items.has_deck_index(0) {
                hand.has_regular_card = false;
            }

            hand.staged_items.clear();

            player_ids.push(id);
        }

        // resolve card damage for the mutate step
        for id in player_ids {
            let entities = &mut simulation.entities;
            let Ok((character, namespace)) =
                entities.query_one_mut::<(&mut Character, &PackageNamespace)>(id)
            else {
                continue;
            };

            let namespace = *namespace;
            let mut cards = std::mem::take(&mut character.cards);

            for card in &mut cards {
                card.damage =
                    CardDamageResolver::new(game_io, resources, namespace, &card.package_id)
                        .resolve(game_io, resources, simulation, id.into())
                        + card.boosted_damage;
            }

            let entities = &mut simulation.entities;
            if let Ok(character) = entities.query_one_mut::<&mut Character>(id) {
                character.cards = cards;
            }
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
        for (_, (player, hand)) in entities.query_mut::<(&mut Player, &PlayerHand)>() {
            let selection = &mut self.player_selections[player.index];
            let selected_item = resolve_selected_item(player, hand, selection);

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
            .map(|(id, _player)| id.into())
            .collect::<Vec<EntityId>>();

        let entities = &mut simulation.entities;

        for id in player_id_vec {
            let (player, hand) = entities
                .query_one_mut::<(&Player, &PlayerHand)>(id.into())
                .unwrap();

            let card_view_size = hand.deck.len().min(player.hand_size());

            for index in 0..card_view_size {
                let (card, namespace) = &hand.deck[index];
                let globals = Globals::from_resources(game_io);

                let card_packages = &globals.card_packages;

                let is_dark = card_packages
                    .package_or_fallback(*namespace, &card.package_id)
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

                move_card_selection(player, hand, selection, x_difference, y_difference);

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

        let Ok((player, hand)) =
            entities.query_one_mut::<(&Player, &PlayerHand)>(simulation.local_player_id.into())
        else {
            return;
        };

        let selection = &mut self.player_selections[player.index];
        let SelectedItem::Card(deck_index) = resolve_selected_item(player, hand, selection) else {
            return;
        };

        let (card, namespace) = &hand.deck[deck_index];

        let globals = Globals::from_resources(game_io);

        let card_packages = &globals.card_packages;

        let is_dark = card_packages
            .package_or_fallback(*namespace, &card.package_id)
            .is_some_and(|package| package.card_properties.card_class == CardClass::Dark);

        if is_dark {
            let globals = Globals::from_resources(game_io);

            resources.ui_fade_color.set(Color::new(0.0, 0.0, 0.0, 0.5));

            simulation.play_sound_with_behavior(
                game_io,
                &globals.sfx.dark_card,
                AudioBehavior::NoOverlap,
            );
        }
    }
}

fn resolve_selected_item(
    player: &Player,
    hand: &PlayerHand,
    selection: &CardSelectSelection,
) -> SelectedItem {
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

    let card_view_size = hand.deck.len().min(player.hand_size());
    let card_index = row * CARD_COLS + col;

    if card_index < card_view_size {
        return SelectedItem::Card(card_index);
    }

    // todo: Card

    SelectedItem::None
}

fn can_player_select(hand: &PlayerHand, index: usize) -> bool {
    if hand.staged_items.visible_count() >= 5 {
        return false;
    }

    let restriction = CardSelectRestriction::resolve(hand);
    let (card, _) = &hand.deck[index];

    restriction.allows_card(card)
}

fn move_card_selection(
    player: &Player,
    hand: &PlayerHand,
    selection: &mut CardSelectSelection,
    col_diff: i32,
    row_diff: i32,
) {
    let previous_item = resolve_selected_item(player, hand, selection);

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
        if resolve_selected_item(player, hand, selection) == SelectedItem::None {
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

            let item = resolve_selected_item(player, hand, selection);

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

        let item = resolve_selected_item(player, hand, selection);

        if item == SelectedItem::None {
            if test_other_rows {
                let previous_row = selection.row;

                for row in 0..CARD_SELECT_ROWS as i32 {
                    selection.row = row;

                    if selection.row == previous_row {
                        continue;
                    }

                    if resolve_selected_item(player, hand, selection) != SelectedItem::None {
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
