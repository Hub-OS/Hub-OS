use super::State;
use crate::battle::*;
use crate::bindable::*;
use crate::ease::inverse_lerp;
use crate::packages::PackageId;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use crate::scenes::BattleEvent;
use enum_map::enum_map;
use enum_map::Enum;
use framework::prelude::*;
use std::sync::Arc;

const FORM_LIST_ANIMATION_TIME: FrameTime = 9;
const FORM_FADE_DELAY: FrameTime = 10;
const FORM_FADE_TIME: FrameTime = 20;
const CARD_COLS: usize = CARD_SELECT_CARD_COLS;

#[derive(Enum, Clone, Copy, PartialEq, Eq)]
enum UiPoint {
    CardStart,
    Confirm,
    SpecialButton,
    Preview,
    FormListStart,
    SelectedCardStart,
}

#[derive(Default, Clone, Copy, PartialEq, Eq)]
enum HistoryHint {
    #[default]
    Card,
    CardButton,
    SpecialButton,
}

#[derive(Clone, Default)]
struct Selection {
    col: i32,
    row: i32,
    form_row: usize,
    card_button_width: usize,
    has_special_button: bool,
    form_select_time: Option<FrameTime>,
    selected_form_index: Option<usize>,
    selected_card_indices: Vec<usize>,
    history: Vec<HistoryHint>,
    form_open_time: Option<FrameTime>,
    confirm_time: FrameTime,
    animating_slide: bool,
    erased: bool,
    local: bool,
}

#[derive(PartialEq, Eq)]
enum SelectedItem {
    Card(usize),
    CardButton,
    SpecialButton,
    Confirm,
    None,
}

#[derive(Clone, Copy)]
enum CardRestriction<'a> {
    Code(&'a str),
    Package(&'a PackageId),
    Mixed {
        package_id: &'a PackageId,
        code: &'a str,
    },
    Any,
}

#[derive(Clone)]
pub struct CardSelectState {
    sprites: Tree<SpriteNode>,
    texture: Arc<Texture>,
    view_index: GenerationalIndex,
    form_list_index: GenerationalIndex,
    animator: Animator,
    points: ResolvedPoints<UiPoint>,
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

            // sfx
            let globals = game_io.resource::<Globals>().unwrap();
            simulation.play_sound(game_io, &globals.sfx.card_select_open);

            // initialize selections
        }

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

        for i in 0..self.player_selections.len() {
            self.animate_slide(simulation, i);
        }

        self.animate_form_list(simulation);
        self.update_buttons(simulation);

        let all_confirmed = (self.player_selections).iter().all(|selection| {
            selection.erased || (!selection.animating_slide && selection.confirm_time != 0)
        });

        if all_confirmed {
            self.complete(game_io, resources, simulation);
        }

        self.time += 1;
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

        let Ok((entity, player)) = entities.query_one_mut::<(&Entity, &Player)>(entity_id.into())
        else {
            return;
        };

        let player_team = entity.team;
        let selection = &self.player_selections[player.index];

        // draw sprite tree
        self.sprites.draw(sprite_queue);

        // drawing hand
        let mut recycled_sprite = Sprite::new(game_io, self.texture.clone());

        // draw icons
        self.animator.set_state("ICON_FRAME");
        self.animator.apply(&mut recycled_sprite);
        let maxed_card_usage = selection.selected_card_indices.len() >= 5;
        let card_restriction = resolve_card_restriction(player, selection);

        for (i, card, position) in self.card_icon_render_iter(player) {
            sprite_queue.set_shader_effect(SpriteShaderEffect::Default);

            recycled_sprite.set_position(position);
            sprite_queue.draw_sprite(&recycled_sprite);

            if selection.selected_card_indices.contains(&i) {
                continue;
            }

            if maxed_card_usage || !does_restriction_allow_card(card_restriction, card) {
                sprite_queue.set_shader_effect(SpriteShaderEffect::Greyscale);
            } else {
                sprite_queue.set_shader_effect(SpriteShaderEffect::Default);
            }
            card.draw_icon(game_io, sprite_queue, position);
        }

        sprite_queue.set_shader_effect(SpriteShaderEffect::Default);

        // draw codes
        const CODE_HORIZONTAL_OFFSET: f32 = 4.0;
        const CODE_VERTICAL_OFFSET: f32 = 16.0;

        let mut code_style = TextStyle::new(game_io, FontStyle::Wide);
        code_style.color = Color::YELLOW;

        for (_, card, position) in self.card_icon_render_iter(player) {
            code_style.bounds.set_position(position);

            code_style.bounds.x += CODE_HORIZONTAL_OFFSET;
            code_style.bounds.y += CODE_VERTICAL_OFFSET;

            code_style.draw(game_io, sprite_queue, &card.code);
        }

        // draw selected icons
        self.animator.set_state("SELECTED_FRAME");
        self.animator.apply(&mut recycled_sprite);

        for (card, position) in self.selected_icon_render_iter(player, selection) {
            recycled_sprite.set_position(position);
            sprite_queue.draw_sprite(&recycled_sprite);

            card.draw_icon(game_io, sprite_queue, position);
        }

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

            if self.time > time + FORM_LIST_ANIMATION_TIME {
                // draw frames
                self.animator.set_state("FORM_FRAME");
                self.animator.apply(&mut recycled_sprite);

                let row_height = recycled_sprite.bounds().height;
                let mut offset = self.points[UiPoint::FormListStart];

                for _ in player.available_forms() {
                    recycled_sprite.set_position(offset);
                    sprite_queue.draw_sprite(&recycled_sprite);

                    offset.y += row_height;
                }

                // draw mugs
                let mut mug_sprite = Sprite::new(game_io, recycled_sprite.texture().clone());

                let mut offset = self.points[UiPoint::FormListStart];

                for (row, (index, form)) in player.available_forms().enumerate() {
                    // default to greyscale
                    sprite_queue.set_shader_effect(SpriteShaderEffect::Greyscale);

                    if selection.selected_form_index == Some(index) {
                        // selected color
                        mug_sprite.set_color(Color::new(0.16, 1.0, 0.87, 1.0));
                    } else if selection.form_row == row {
                        // hovered option is plain
                        sprite_queue.set_shader_effect(SpriteShaderEffect::Default);
                        mug_sprite.set_color(Color::WHITE);
                    } else {
                        // default color darkens the image
                        mug_sprite.set_color(Color::new(0.61, 0.68, 0.71, 1.0));
                    }

                    if let Some(texture) = form.mug_texture.as_ref() {
                        mug_sprite.set_texture(texture.clone());
                        mug_sprite.set_position(offset);
                        sprite_queue.draw_sprite(&mug_sprite);
                    }

                    offset.y += row_height;
                }

                // reset shader effect
                sprite_queue.set_shader_effect(SpriteShaderEffect::Default);

                // draw cursor
                if selection.form_select_time.is_none() {
                    let mut offset = self.points[UiPoint::FormListStart];
                    offset.y += row_height * selection.form_row as f32;

                    self.animator.set_state("FORM_CURSOR");
                    self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                    self.animator.sync_time(self.time);

                    self.animator.apply(&mut recycled_sprite);
                    recycled_sprite.set_position(offset);
                    sprite_queue.draw_sprite(&recycled_sprite);
                }
            }
        } else {
            // draw card selection

            // draw cursor
            let selected_item = resolve_selected_item(player, selection);

            if selection.confirm_time == 0 {
                let animator = &mut self.animator;
                let root_offset = self.sprites.root().offset();
                let mut draw_state_at = |state: &str, position: Vec2| {
                    animator.set_state(state);
                    animator.set_loop_mode(AnimatorLoopMode::Loop);
                    animator.sync_time(self.time);
                    animator.apply(&mut recycled_sprite);
                    recycled_sprite.set_position(position + root_offset);
                    sprite_queue.draw_sprite(&recycled_sprite);
                };

                match selected_item {
                    SelectedItem::Card(_) => {
                        let position = calculate_icon_position(
                            self.points[UiPoint::CardStart],
                            selection.col,
                            selection.row,
                        );

                        draw_state_at("CARD_CURSOR", position);
                    }
                    SelectedItem::Confirm => {
                        draw_state_at("CONFIRM_CURSOR", self.points[UiPoint::Confirm]);
                    }
                    SelectedItem::SpecialButton => {
                        draw_state_at("SPECIAL_CURSOR", self.points[UiPoint::SpecialButton]);
                    }
                    SelectedItem::CardButton => {
                        let button_width = selection.card_button_width;
                        let start_position = calculate_icon_position(
                            self.points[UiPoint::CardStart],
                            CARD_COLS.saturating_sub(button_width) as i32,
                            CARD_SELECT_ROWS as i32 - 1,
                        );

                        let end_position = calculate_icon_position(
                            self.points[UiPoint::CardStart],
                            CARD_COLS as i32,
                            CARD_SELECT_ROWS as i32 - 1,
                        );

                        draw_state_at("CARD_BUTTON_LEFT_CURSOR", start_position);
                        draw_state_at("CARD_BUTTON_RIGHT_CURSOR", end_position);
                    }
                    _ => {}
                }

                if player.has_regular_card {
                    draw_state_at("REGULAR_FRAME", Vec2::ZERO);
                }
            }

            // draw preview icon
            let preview_point = self.points[UiPoint::Preview] + self.sprites.root().offset();

            match selected_item {
                SelectedItem::Card(i) => {
                    let card = &player.deck[i];
                    card.draw_preview(game_io, sprite_queue, preview_point, 1.0);
                    card.draw_preview_title(game_io, sprite_queue, preview_point);
                }
                SelectedItem::Confirm => {
                    let player_selection = self.player_selections.get(player.index);
                    let selected_a_card = player_selection
                        .map(|selection| !selection.selected_card_indices.is_empty())
                        .unwrap_or_default();

                    if selected_a_card {
                        self.animator.set_state("CONFIRM_MESSAGE");
                    } else {
                        self.animator.set_state("EMPTY_MESSAGE");
                    }

                    self.animator.apply(&mut recycled_sprite);
                    recycled_sprite.set_position(preview_point);
                    sprite_queue.draw_sprite(&recycled_sprite);
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
            let mut background_sprite = recycled_sprite.clone();
            let mut edge_sprite = recycled_sprite;

            self.animator.set_state("NAME_PLATE");
            self.animator.apply(&mut background_sprite);

            self.animator.set_state("NAME_PLATE_EDGE");
            self.animator.apply(&mut edge_sprite);

            let mut style = TextStyle::new(game_io, FontStyle::Thick);
            let mut y = 3.0;

            // draw names
            for (_, (entity, character)) in simulation.entities.query_mut::<(&Entity, &Character)>()
            {
                if entity.team == player_team {
                    continue;
                }

                let name = &entity.name;
                let suffix = character.rank.suffix();

                // calculate size + position for text
                let name_width = style.measure(name).size.x + 1.0;
                let text_width = name_width + style.measure(suffix).size.x;

                style.bounds.x = RESOLUTION_F.x - text_width - 1.0;
                style.bounds.y = y + 1.0;

                // draw plate
                let background_position = Vec2::new(style.bounds.x, y);

                background_sprite.set_position(background_position);
                background_sprite.set_width(text_width + 1.0);
                sprite_queue.draw_sprite(&background_sprite);

                edge_sprite.set_position(background_position);
                sprite_queue.draw_sprite(&edge_sprite);

                // draw name on top of plate
                style.draw(game_io, sprite_queue, name);

                // draw suffix after name
                style.bounds.x += name_width;
                style.draw(game_io, sprite_queue, suffix);

                y += 16.0;
            }
        }

        if !selection.animating_slide && selection.confirm_time != 0 {
            // render text to signal we're waiting on other players
            const MARGIN_TOP: f32 = 38.0;

            let wait_time = self.time - selection.confirm_time;

            if (wait_time / 30) % 2 == 0 {
                const TEXT: &str = "Waiting...";

                let mut style = TextStyle::new(game_io, FontStyle::Thick);
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

            let assets = &game_io.resource::<Globals>().unwrap().assets;

            let mut fade_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
            fade_sprite.set_bounds(Rect::from_corners(Vec2::ZERO, RESOLUTION_F));
            fade_sprite.set_color(Color::WHITE.multiply_alpha(a));
            sprite_queue.draw_sprite(&fade_sprite);
        }
    }
}

impl CardSelectState {
    pub fn new(game_io: &GameIO) -> Self {
        let mut sprites = Tree::new(SpriteNode::new(game_io, SpriteColorMode::Multiply));

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let mut animator = Animator::load_new(assets, ResourcePaths::BATTLE_CARD_SELECT_ANIMATION);
        let texture = assets.texture(game_io, ResourcePaths::BATTLE_CARD_SELECT);

        let root_node = sprites.root_mut();
        root_node.set_texture_direct(texture.clone());
        animator.set_state("ROOT");
        root_node.set_layer(-1);
        root_node.apply_animation(&animator);

        // card frame
        animator.set_state("STANDARD_FRAME");

        let mut card_frame_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
        card_frame_node.set_texture_direct(texture.clone());
        card_frame_node.apply_animation(&animator);
        let view_index = sprites.insert_root_child(card_frame_node);

        // start form list frame as the tab
        animator.set_state("FORM_TAB");

        let mut form_list_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
        form_list_node.set_texture_direct(texture.clone());
        form_list_node.apply_animation(&animator);
        form_list_node.set_visible(false);
        let form_list_index = sprites.insert_root_child(form_list_node);

        // selection
        animator.set_state("SELECTION_FRAME");

        let mut selection_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
        selection_node.set_texture_direct(texture.clone());
        selection_node.apply_animation(&animator);
        sprites.insert_root_child(selection_node);

        let points = ResolvedPoints::new_parented(
            &mut animator,
            enum_map! {
                UiPoint::CardStart => ("ROOT", "CARD_START"),
                UiPoint::Confirm => ("ROOT", "CONFIRM"),
                UiPoint::SpecialButton => ("ROOT", "SPECIAL"),
                UiPoint::FormListStart => ("FORM_LIST_FRAME", "START"),
                UiPoint::Preview => ("STANDARD_FRAME", "PREVIEW"),
                UiPoint::SelectedCardStart => ("SELECTION_FRAME", "START"),
            },
            |_| None,
        );

        Self {
            sprites,
            texture,
            view_index,
            form_list_index,
            animator,
            points,
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
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
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
            if let Some((index, _)) = player.available_forms().nth(selection.form_row) {
                if selection.selected_form_index == Some(index) {
                    // deselect the form if the player reselected it
                    selection.selected_form_index = None;
                } else {
                    // select new form
                    selection.selected_form_index = Some(index);
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
                if let Some((_, form)) = player.available_forms().nth(selection.form_row) {
                    let event = BattleEvent::Description((*form.description).clone());
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
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let mut pending_sfx = Vec::new();

        let player_index = player.index;
        let input = &simulation.inputs[player_index];
        let selection = &mut self.player_selections[player_index];

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
                    selection.history.clear();
                    selection.confirm_time = self.time;

                    // sfx
                    if selection.local {
                        pending_sfx.push(&globals.sfx.card_select_confirm);
                    }
                }
                SelectedItem::Card(index) => {
                    if !selection.selected_card_indices.contains(&index)
                        && can_player_select(player, selection, index)
                    {
                        selection.history.push(HistoryHint::Card);
                        selection.selected_card_indices.push(index);

                        // sfx
                        if selection.local {
                            pending_sfx.push(&globals.sfx.cursor_select);
                        }
                    } else if selection.local {
                        // error sfx
                        pending_sfx.push(&globals.sfx.cursor_error);
                    }
                }
                SelectedItem::CardButton => {
                    self.use_button(
                        game_io,
                        resources,
                        simulation,
                        entity_id,
                        HistoryHint::CardButton,
                    );
                }
                SelectedItem::SpecialButton => {
                    self.use_button(
                        game_io,
                        resources,
                        simulation,
                        entity_id,
                        HistoryHint::SpecialButton,
                    );
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
            // clear selection
            selection.selected_card_indices.clear();
            selection.selected_form_index.take();

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
                if let SelectedItem::Card(index) = selected_item {
                    let entities = &mut simulation.entities;
                    let player = entities.query_one_mut::<&Player>(entity_id.into()).unwrap();

                    let card = &player.deck[index];

                    let event = BattleEvent::DescribeCard(card.package_id.clone());
                    resources.event_sender.send(event).unwrap();
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
        history_hint: HistoryHint,
    ) {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let card_button =
            PlayerOverridables::flat_map_for(player, move |overridables| match history_hint {
                HistoryHint::CardButton => overridables.card_button.as_ref(),
                HistoryHint::SpecialButton => overridables.special_button.as_ref(),
                _ => unreachable!(),
            })
            .next();

        let Some(button) = card_button else {
            return;
        };

        let Some(callback) = button.use_callback.clone() else {
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let selection = &mut self.player_selections[player.index];
        let can_undo = button.undo_callback.is_some();

        let success = callback.call(game_io, resources, simulation, ());

        if success {
            if can_undo {
                selection.history.push(history_hint);
            } else {
                selection.history.clear();
            }

            if selection.local {
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
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let selection = &mut self.player_selections[player.index];
        let hint = selection.history.pop().unwrap_or_default();
        let mut applied = false;

        match hint {
            HistoryHint::Card => {
                if !selection.selected_card_indices.is_empty() {
                    selection.selected_card_indices.pop();
                    applied = true;
                } else if selection.selected_form_index.is_some() {
                    selection.selected_form_index = None;
                    applied = true;

                    if selection.local {
                        simulation.play_sound(game_io, &globals.sfx.transform_revert);
                    }
                }
            }
            HistoryHint::CardButton | HistoryHint::SpecialButton => {
                let card_button =
                    PlayerOverridables::flat_map_for(player, move |overridables| match hint {
                        HistoryHint::CardButton => overridables.card_button.as_ref(),
                        HistoryHint::SpecialButton => overridables.special_button.as_ref(),
                        _ => unreachable!(),
                    })
                    .next();

                if let Some(callback) = card_button.and_then(|button| button.undo_callback.clone())
                {
                    callback.call(game_io, resources, simulation, ());
                }
            }
        }

        // sfx
        if selection.local {
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
        let card_packages = &game_io.resource::<Globals>().unwrap().card_packages;

        let entities = &mut simulation.entities;
        for (_, (player, character)) in entities.query_mut::<(&mut Player, &mut Character)>() {
            let selection = &mut self.player_selections[player.index];

            if selection.selected_form_index.is_some() {
                // change form
                player.active_form = selection.selected_form_index;
            }

            if selection.selected_card_indices.is_empty() {
                // if no cards were selected, we keep cards from the previous selection
                continue;
            }

            character.cards.clear();
            character.next_card_mutation = Some(0);

            // load cards in reverse as we'll pop them off in battle state (first item must be last)
            for i in selection.selected_card_indices.iter().rev() {
                let namespace = player.namespace();

                let card = &player.deck[*i];

                let mut card_properties = card_packages
                    .package_or_override(namespace, &card.package_id)
                    .map(|package| {
                        let status_registry = &resources.status_registry;

                        package.card_properties.to_bindable(status_registry)
                    })
                    .unwrap_or_default();

                card_properties.code = card.code.clone();
                card_properties.namespace = Some(namespace);
                character.cards.push(card_properties);
            }

            // remove the cards from the deck
            // must sort + loop in reverse to prevent issues from shifting indices
            selection.selected_card_indices.sort();

            for i in selection.selected_card_indices.iter().rev() {
                player.deck.remove(*i);
            }

            // unmark player as having regular card
            if player.has_regular_card && selection.selected_card_indices.contains(&0) {
                player.has_regular_card = false;
            }

            selection.selected_card_indices.clear();
        }

        Character::mutate_cards(game_io, resources, simulation);

        self.completed = true;
    }

    fn update_buttons(&mut self, simulation: &mut BattleSimulation) {
        let entities = &mut simulation.entities;
        let sprite_trees = &mut simulation.sprite_trees;
        let animators = &mut simulation.animators;
        let pending_callbacks = &mut simulation.pending_callbacks;

        let root_offset = self.sprites.root().offset();

        let mut preview_position = self.points[UiPoint::Preview] + root_offset;
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
                let position = calculate_icon_position(
                    self.points[UiPoint::CardStart],
                    CARD_COLS.saturating_sub(button_width) as i32,
                    CARD_SELECT_ROWS as i32 - 1,
                ) + root_offset;

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
                let position = self.points[UiPoint::SpecialButton] + root_offset;
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

    fn animate_form_list(&mut self, simulation: &mut BattleSimulation) {
        let mut selections_iter = self.player_selections.iter();
        let Some(selection) = selections_iter.find(|selection| selection.local) else {
            return;
        };

        let form_list_node = &mut self.sprites[self.form_list_index];

        // make visible if there's forms available
        if let Ok(player) = simulation
            .entities
            .query_one_mut::<&Player>(simulation.local_player_id.into())
        {
            if player.available_forms().next().is_some() {
                form_list_node.set_visible(true);
            } else {
                // no need to animate
                return;
            }
        };

        let Some(time) = selection.form_open_time else {
            self.animator.set_state("FORM_TAB");
            form_list_node.apply_animation(&self.animator);
            return;
        };

        self.animator.set_state("FORM_LIST_FRAME");
        form_list_node.apply_animation(&self.animator);

        // animate origin
        let inital_origin = self.animator.point("initial_origin").unwrap_or_default();
        let progress = inverse_lerp!(time, time + FORM_LIST_ANIMATION_TIME, self.time);
        let origin = inital_origin.lerp(self.animator.origin(), progress);
        form_list_node.set_origin(origin)
    }

    fn animate_slide(&mut self, simulation: &mut BattleSimulation, player_index: usize) {
        const ANIMATION_DURATION: f32 = 10.0;

        let selection = &mut self.player_selections[player_index];

        let progress = if selection.confirm_time == 0 {
            inverse_lerp!(0.0, ANIMATION_DURATION, self.time)
        } else {
            1.0 - inverse_lerp!(0.0, ANIMATION_DURATION, self.time - selection.confirm_time)
        };

        if selection.local {
            // update ui if this is the local selection
            let root_node = self.sprites.root_mut();
            let width = root_node.size().x;

            root_node.set_offset(Vec2::new((1.0 - progress) * -width, 0.0));

            // note: moving health ui also moves emotion ui
            (simulation.local_health_ui)
                .set_position(Vec2::new(progress * width + BATTLE_UI_MARGIN, 0.0));
        }

        // returns true if we're still animating
        selection.animating_slide = if selection.confirm_time == 0 {
            progress < 1.0
        } else {
            // animation is reversed when every player has confirmed
            progress > 0.0
        };
    }

    fn hide(&mut self, simulation: &mut BattleSimulation) {
        let root_node = self.sprites.root_mut();
        root_node.set_visible(false);

        // note: moving health ui also moves emotion ui
        (simulation.local_health_ui).set_position(Vec2::new(BATTLE_UI_MARGIN, 0.0));
    }

    fn reveal(&mut self, simulation: &mut BattleSimulation) {
        let root_node = self.sprites.root_mut();
        root_node.set_visible(true);

        // note: moving health ui also moves emotion ui
        let width = root_node.size().x;
        (simulation.local_health_ui).set_position(Vec2::new(width + BATTLE_UI_MARGIN, 0.0));
    }

    fn card_icon_render_iter<'a>(
        &self,
        player: &'a Player,
    ) -> impl Iterator<Item = (usize, &'a Card, Vec2)> {
        let mut start = self.points[UiPoint::CardStart];
        start.x += self.sprites.root().offset().x;

        // draw icons
        player
            .deck
            .iter()
            .take(player.hand_size())
            .enumerate()
            .map(move |(i, card)| {
                let col = i % CARD_COLS;
                let row = i / CARD_COLS;

                let top_left = calculate_icon_position(start, col as i32, row as i32);

                (i, card, top_left)
            })
    }

    fn calculate_icon_position(&self, col: i32, row: i32) -> Vec2 {
        let mut start = self.points[UiPoint::CardStart];
        start.x += self.sprites.root().offset().x;

        calculate_icon_position(start, col, row)
    }

    fn selected_icon_render_iter<'a>(
        &self,
        player: &'a Player,
        selection: &'a Selection,
    ) -> impl Iterator<Item = (&'a Card, Vec2)> {
        const VERTICAL_OFFSET: f32 = 16.0;

        let mut start = self.points[UiPoint::SelectedCardStart];
        start.x += self.sprites.root().offset().x;

        // draw icons
        selection
            .selected_card_indices
            .iter()
            .enumerate()
            .map(move |(i, card_index)| {
                let card = &player.deck[*card_index];

                let position = Vec2::new(start.x, start.y + VERTICAL_OFFSET * i as f32);

                (card, position)
            })
    }
}

fn calculate_icon_position(start: Vec2, col: i32, row: i32) -> Vec2 {
    const HORIZONTAL_DIFF: f32 = 16.0;
    const VERTICAL_DIFF: f32 = 24.0;

    Vec2::new(
        start.x + col as f32 * HORIZONTAL_DIFF,
        start.y + row as f32 * VERTICAL_DIFF,
    )
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

fn can_player_select(player: &Player, selection: &Selection, index: usize) -> bool {
    if selection.selected_card_indices.len() >= 5 {
        return false;
    }

    let restriction = resolve_card_restriction(player, selection);
    let card = &player.deck[index];

    does_restriction_allow_card(restriction, card)
}

fn does_restriction_allow_card(restriction: CardRestriction, card: &Card) -> bool {
    match restriction {
        CardRestriction::Code(code) => card.code == code || card.code == "*",
        CardRestriction::Package(package_id) => card.package_id == *package_id,
        CardRestriction::Mixed { package_id, code } => {
            card.package_id == *package_id || card.code == code || card.code == "*"
        }
        CardRestriction::Any => true,
    }
}

fn resolve_card_restriction<'a>(player: &'a Player, selection: &Selection) -> CardRestriction<'a> {
    if selection.selected_card_indices.is_empty() {
        return CardRestriction::Any;
    }

    let mut first_package_id: Option<&PackageId> = None;
    let mut code_restriction: Option<&str> = None;

    let mut same_package_id = true;
    let mut same_code = true;

    for i in selection.selected_card_indices.iter().cloned() {
        let card = &player.deck[i];

        if let Some(package_id) = first_package_id {
            same_package_id &= card.package_id == *package_id;
        } else {
            first_package_id = Some(&card.package_id);
        }

        if let Some(code) = code_restriction {
            same_code &= card.code == code || card.code == "*";
        } else if card.code != "*" {
            code_restriction = Some(&card.code);
        }
    }

    if let Some(code) = code_restriction {
        if same_package_id {
            if !same_code {
                return CardRestriction::Package(first_package_id.unwrap());
            } else {
                return CardRestriction::Mixed {
                    package_id: first_package_id.unwrap(),
                    code,
                };
            }
        }

        if same_code {
            return CardRestriction::Code(code);
        }
    }

    CardRestriction::Any
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
