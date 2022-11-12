use super::State;
use crate::battle::*;
use crate::bindable::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use framework::prelude::*;
use std::sync::Arc;

#[derive(Clone, Default)]
struct Selection {
    col: i32,
    row: i32,
    form_index: usize,
    selected_form_index: Option<usize>,
    selected_card_indices: Vec<usize>,
    confirm_time: FrameTime,
    animating_slide: bool,
    erased: bool,
    local: bool,
}

#[derive(PartialEq, Eq)]
enum SelectedItem {
    Card(usize),
    CardButton,
    WideButton,
    SpecialButton,
    Confirm,
    None,
}

#[derive(Clone, Copy)]
enum CardRestriction<'a> {
    Code(&'a str),
    Package(&'a str),
    Mixed { package_id: &'a str, code: &'a str },
    Any,
}

#[derive(Clone)]
pub struct CardSelectState {
    sprites: Tree<SpriteNode>,
    texture: Arc<Texture>,
    view_index: GenerationalIndex,
    animator: Animator,
    card_start: Vec2,
    confirm_point: Vec2,
    preview_point: Vec2,
    selected_card_start: Vec2,
    player_selections: Vec<Selection>,
    time: FrameTime,
    completed: bool,
}

impl State for CardSelectState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, _: &GameIO<Globals>) -> Option<Box<dyn State>> {
        if self.completed {
            Some(Box::new(BattleState::new()))
        } else {
            None
        }
    }

    fn update(
        &mut self,
        game_io: &GameIO<Globals>,
        _shared_assets: &mut SharedBattleAssets,
        simulation: &mut BattleSimulation,
        _vms: &[RollbackVM],
    ) {
        if self.time == 0 {
            simulation.statistics.turns += 1;
            simulation.turn_gauge.set_time(0);

            // sfx
            let globals = game_io.globals();
            simulation.play_sound(game_io, &globals.card_select_open_sfx);

            // initialize selections
        }

        for selection in &mut self.player_selections {
            // assume erased until we hit the next loop
            selection.erased = true;
        }

        // we'll only see our own selection
        // but we must simulate every player's input to stay in sync
        for (_, player) in (simulation.entities).query_mut::<&Player>() {
            if player.index >= self.player_selections.len() {
                self.player_selections
                    .resize_with(player.index + 1, Selection::default);

                // initialize selection
                let selection = &mut self.player_selections[player.index];
                selection.local = player.local;
                selection.animating_slide = true;
            }

            let selection = &mut self.player_selections[player.index];

            // unmark as erased
            selection.erased = false;

            if selection.animating_slide || selection.confirm_time != 0 {
                // if we're animating the ui or already confirmed we should skip this input
                continue;
            }

            let input = &simulation.inputs[player.index];

            let previous_item = resolve_selected_item(player, selection);

            if previous_item == SelectedItem::None {
                // select Confirm as a safety net
                selection.col = 5;
                selection.row = 0;
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
            if previous_item != selected_item && selection.local && !simulation.is_resimulation {
                let globals = game_io.globals();
                globals.audio.play_sound(&globals.cursor_move_sfx);
            }

            if input.was_just_pressed(Input::Confirm) {
                match selected_item {
                    SelectedItem::Confirm => {
                        selection.confirm_time = self.time;

                        // sfx
                        if selection.local && !simulation.is_resimulation {
                            let globals = game_io.globals();
                            globals.audio.play_sound(&globals.card_select_confirm_sfx);
                        }
                    }
                    SelectedItem::Card(index) => {
                        if !selection.selected_card_indices.contains(&index)
                            && can_player_select(player, selection, index)
                        {
                            selection.selected_card_indices.push(index);

                            // sfx
                            if selection.local && !simulation.is_resimulation {
                                let globals = game_io.globals();
                                globals.audio.play_sound(&globals.cursor_select_sfx);
                            }
                        } else if selection.local && !simulation.is_resimulation {
                            // error sfx
                            let globals = game_io.globals();
                            globals.audio.play_sound(&globals.cursor_error_sfx);
                        }
                    }
                    _ => {}
                }
            }

            if input.was_just_pressed(Input::Cancel) && !selection.selected_card_indices.is_empty()
            {
                selection.selected_card_indices.pop();

                // sfx
                if selection.local && !simulation.is_resimulation {
                    let globals = game_io.globals();
                    globals.audio.play_sound(&globals.cursor_cancel_sfx);
                }
            }

            // todo: form selection
        }

        for i in 0..self.player_selections.len() {
            self.animate_slide(simulation, i);
        }

        let all_confirmed = (self.player_selections).iter().all(|selection| {
            selection.erased || (!selection.animating_slide && selection.confirm_time != 0)
        });

        if all_confirmed {
            self.complete(game_io, simulation);
        }

        self.time += 1;
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO<Globals>,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        if self.completed {
            return;
        }

        let (entity, player) = match (simulation.entities)
            .query_one_mut::<(&Entity, &Player)>(simulation.local_player_id.into())
        {
            Ok(player) => player,
            _ => return,
        };

        let player_team = entity.team;
        let selection = &self.player_selections[player.index];

        // create queue
        let globals = game_io.globals();
        let default_sampler = globals.default_sampler.clone();

        // draw sprite tree
        self.sprites.draw(sprite_queue);

        let mut recycled_sprite = Sprite::new(self.texture.clone(), default_sampler);
        // drawing hand

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

        // draw cursor
        let selected_item = resolve_selected_item(player, selection);

        if selection.confirm_time == 0 {
            match selected_item {
                SelectedItem::Card(_) => {
                    self.animator.set_state("CARD_CURSOR");
                    self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                    self.animator.sync_time(self.time);

                    let offset = self.calculate_icon_position(selection.col, selection.row);

                    self.animator.apply(&mut recycled_sprite);
                    recycled_sprite.set_position(offset);
                    sprite_queue.draw_sprite(&recycled_sprite);
                }
                SelectedItem::Confirm => {
                    self.animator.set_state("SIDE_CURSOR");
                    self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                    self.animator.sync_time(self.time);

                    self.animator.apply(&mut recycled_sprite);
                    recycled_sprite.set_position(self.confirm_point);
                    sprite_queue.draw_sprite(&recycled_sprite);
                }
                _ => {}
            }
        }

        // draw preview icon
        let preview_point = self.preview_point + self.sprites.root().offset();

        match selected_item {
            SelectedItem::Card(i) => {
                let card = &player.cards[i];
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
            SelectedItem::CardButton
            | SelectedItem::WideButton
            | SelectedItem::SpecialButton
            | SelectedItem::None => {
                // unreachable, currently
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
    }
}

impl CardSelectState {
    pub fn new(game_io: &GameIO<Globals>) -> Self {
        let mut sprites = Tree::new(SpriteNode::new(game_io, SpriteColorMode::Multiply));

        let globals = game_io.globals();
        let assets = &globals.assets;
        let mut animator = Animator::load_new(assets, ResourcePaths::BATTLE_CARD_SELECT_ANIMATION);
        let texture = assets.texture(game_io, ResourcePaths::BATTLE_CARD_SELECT);

        let root_node = sprites.root_mut();
        root_node.set_texture_direct(texture.clone());
        animator.set_state("ROOT");
        root_node.apply_animation(&animator);

        // points
        let origin = animator.origin();
        let card_start = animator.point("card_start").unwrap_or_default() - origin;
        let confirm_point = animator.point("confirm").unwrap_or_default() - origin;

        // card frame
        animator.set_state("STANDARD_FRAME");

        let origin = animator.origin();
        let preview_point = animator.point("preview").unwrap_or_default() - origin;

        let mut card_frame_node = root_node.clone();
        card_frame_node.set_texture_direct(texture.clone());
        card_frame_node.apply_animation(&animator);
        let view_index = sprites.insert_root_child(card_frame_node);

        // selection
        animator.set_state("SELECTION_FRAME");

        let selection_origin = animator.origin();
        let selected_card_start =
            animator.point("start").unwrap_or_default() - selection_origin - origin;

        let mut selection_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
        selection_node.set_texture_direct(texture.clone());
        selection_node.apply_animation(&animator);
        sprites.insert_root_child(selection_node);

        Self {
            sprites,
            texture,
            view_index,
            animator,
            card_start,
            confirm_point,
            preview_point,
            selected_card_start,
            player_selections: Vec::new(),
            time: 0,
            completed: false,
        }
    }

    fn complete(&mut self, game_io: &GameIO<Globals>, simulation: &mut BattleSimulation) {
        let card_packages = &game_io.globals().card_packages;

        for (_, (player, character)) in
            (simulation.entities).query_mut::<(&mut Player, &mut Character)>()
        {
            let selection = &mut self.player_selections[player.index];

            if selection.selected_card_indices.is_empty() {
                // if no cards were selected, we keep cards from the previous selection
                continue;
            }

            character.cards.clear();

            // load cards in reverse as we'll pop them off in battle state (first item must be last)
            for i in selection.selected_card_indices.iter().rev() {
                let namespace = player.namespace();

                let card = &player.cards[*i];

                if let Some(package) =
                    card_packages.package_or_fallback(namespace, &card.package_id)
                {
                    character.cards.push(package.card_properties.clone());
                }
            }

            // remove the cards from the folder
            // must sort + loop in reverse to prevent issues from shifting indices
            selection.selected_card_indices.sort();

            for i in selection.selected_card_indices.iter().rev() {
                player.cards.remove(*i);
            }

            selection.selected_card_indices.clear();
        }

        self.completed = true;
    }

    fn animate_slide(&mut self, simulation: &mut BattleSimulation, player_index: usize) {
        const ANIMATION_DURATION: f32 = 10.0;

        use crate::ease::inverse_lerp;

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

        (simulation.local_health_ui).set_position(Vec2::new(BATTLE_UI_MARGIN, 0.0));
    }

    fn reveal(&mut self, simulation: &mut BattleSimulation) {
        let root_node = self.sprites.root_mut();
        root_node.set_visible(true);

        let width = root_node.size().x;
        (simulation.local_health_ui).set_position(Vec2::new(width + BATTLE_UI_MARGIN, 0.0));
    }

    fn card_icon_render_iter<'a>(
        &self,
        player: &'a Player,
    ) -> impl Iterator<Item = (usize, &'a Card, Vec2)> {
        let mut start = self.card_start;
        start.x += self.sprites.root().offset().x;

        // draw icons
        player
            .cards
            .iter()
            .take(player.card_view_size.min(10) as usize)
            .enumerate()
            .map(move |(i, card)| {
                let col = i % 5;
                let row = i / 5;

                let top_left = calculate_icon_position(start, col as i32, row as i32);

                (i, card, top_left)
            })
    }

    fn calculate_icon_position(&self, col: i32, row: i32) -> Vec2 {
        let mut start = self.card_start;
        start.x += self.sprites.root().offset().x;

        calculate_icon_position(start, col, row)
    }

    fn selected_icon_render_iter<'a>(
        &self,
        player: &'a Player,
        selection: &'a Selection,
    ) -> impl Iterator<Item = (&'a Card, Vec2)> {
        const VERTICAL_OFFSET: f32 = 16.0;

        let mut start = self.selected_card_start;
        start.x += self.sprites.root().offset().x;

        // draw icons
        selection
            .selected_card_indices
            .iter()
            .enumerate()
            .map(move |(i, card_index)| {
                let card = &player.cards[*card_index];

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
    if selection.row == 0 && selection.col == 5 {
        return SelectedItem::Confirm;
    }

    let card_view_size = player.cards.len().min(player.card_view_size as usize);
    let card_index = (selection.row * 5 + selection.col) as usize;

    if card_index < card_view_size {
        return SelectedItem::Card(card_index);
    }

    // todo: Wide, Card, Special

    SelectedItem::None
}

fn can_player_select(player: &Player, selection: &Selection, index: usize) -> bool {
    if selection.selected_card_indices.len() >= 5 {
        return false;
    }

    let restriction = resolve_card_restriction(player, selection);
    let card = &player.cards[index];

    does_restriction_allow_card(restriction, card)
}

fn does_restriction_allow_card(restriction: CardRestriction, card: &Card) -> bool {
    match restriction {
        CardRestriction::Code(code) => card.code == code || card.code == "*",
        CardRestriction::Package(package_id) => card.package_id == package_id,
        CardRestriction::Mixed { package_id, code } => {
            card.package_id == package_id || card.code == code || card.code == "*"
        }
        CardRestriction::Any => true,
    }
}

fn resolve_card_restriction<'a>(player: &'a Player, selection: &Selection) -> CardRestriction<'a> {
    if selection.selected_card_indices.is_empty() {
        return CardRestriction::Any;
    }

    let mut first_package_id: Option<&str> = None;
    let mut code_restriction: Option<&str> = None;

    let mut same_package_id = true;
    let mut same_code = true;

    for i in selection.selected_card_indices.iter().cloned() {
        let card = &player.cards[i];

        if let Some(package_id) = first_package_id {
            same_package_id &= card.package_id == package_id;
        } else {
            first_package_id = Some(&card.package_id);
        }

        if let Some(code) = code_restriction {
            same_code &= card.code == code || card.code == "*";
        } else if card.code != "*" {
            code_restriction = Some(&card.code);
        }
    }

    if same_package_id && code_restriction.is_some() {
        if !same_code {
            return CardRestriction::Package(first_package_id.unwrap());
        } else {
            return CardRestriction::Mixed {
                package_id: first_package_id.unwrap(),
                code: code_restriction.unwrap(),
            };
        }
    }

    if same_code && code_restriction.is_some() {
        return CardRestriction::Code(code_restriction.unwrap());
    }

    CardRestriction::Any
}

fn move_card_selection(player: &Player, selection: &mut Selection, col_diff: i32, row_diff: i32) {
    let previous_selection = resolve_selected_item(player, selection);
    let mut same_selection_encountered = false;

    // 6 attempts, we should've looped back around by then
    for _ in 0..6 {
        selection.col += col_diff;
        selection.row += row_diff;

        if selection.col == -1 {
            selection.col = 5;
        }

        if selection.row == -1 {
            selection.row = 1;
        }

        selection.col %= 6;
        selection.row %= 2;

        let new_selection = resolve_selected_item(player, selection);

        if new_selection == SelectedItem::None {
            continue;
        }

        if new_selection == previous_selection && !same_selection_encountered {
            // account for wide buttons
            same_selection_encountered = true;
            continue;
        }

        return;
    }
}
