use super::{FontName, TextStyle};
use crate::battle::{
    BattleSimulation, CardSelectRestriction, CardSelectSelection, Character, Entity, EntityName,
    Player, PlayerHand,
};
use crate::bindable::{CardClass, SpriteColorMode};
use crate::packages::{CardPackage, PackageNamespace};
use crate::render::{
    Animator, AnimatorLoopMode, FrameTime, ResolvedPoints, SpriteColorQueue, SpriteNode,
    SpriteShaderEffect,
};
use crate::resources::{
    AssetManager, BATTLE_UI_MARGIN, CARD_SELECT_CARD_COLS as CARD_COLS, Globals, RESOLUTION_F,
    ResourcePaths,
};
use crate::saves::Card;
use crate::structures::{GenerationalIndex, Tree};
use enum_map::{Enum, enum_map};
use framework::prelude::*;

#[derive(Enum, Clone, Copy, PartialEq, Eq)]
enum UiPoint {
    CardStart,
    Confirm,
    SpecialButton,
    Preview,
    FormListStart,
    StagedCardStart,
}

#[derive(Clone)]
pub struct CardSelectUi {
    time: FrameTime,
    sprites: Tree<SpriteNode>,
    recycled_sprite: Sprite,
    animator: Animator,
    points: ResolvedPoints<UiPoint>,
    card_frame_index: GenerationalIndex,
    form_list_index: GenerationalIndex,
}

impl CardSelectUi {
    pub const FORM_LIST_ANIMATION_TIME: FrameTime = 9;
    pub const SLIDE_DURATION: FrameTime = 10;

    pub fn new(game_io: &GameIO) -> Self {
        let mut sprites = Tree::new(SpriteNode::new(game_io, SpriteColorMode::Multiply));

        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let mut animator = Animator::load_new(assets, ResourcePaths::BATTLE_CARD_SELECT_ANIMATION);
        let recycled_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_CARD_SELECT);
        let texture = recycled_sprite.texture();

        let root_node = sprites.root_mut();
        root_node.set_texture_direct(texture.clone());
        animator.set_state("ROOT");
        root_node.set_layer(-1);
        root_node.apply_animation(&animator);

        // card frame
        animator.set_state("STANDARD_FRAME");

        let mut card_frame_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
        card_frame_node.set_layer(2);
        card_frame_node.set_texture_direct(texture.clone());
        card_frame_node.apply_animation(&animator);
        let card_frame_index = sprites.insert_root_child(card_frame_node);

        // start form list frame as the tab
        animator.set_state("FORM_TAB");

        let mut form_list_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
        form_list_node.set_layer(1);
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
                UiPoint::StagedCardStart => ("SELECTION_FRAME", "START"),
            },
            |_| None,
        );

        Self {
            time: 0,
            sprites,
            recycled_sprite,
            animator,
            points,
            card_frame_index,
            form_list_index,
        }
    }

    fn root_offset(&self) -> Vec2 {
        self.sprites.root().offset()
    }

    pub fn preview_point(&self) -> Vec2 {
        self.points[UiPoint::Preview] + self.root_offset()
    }

    pub fn card_start_point(&self) -> Vec2 {
        self.points[UiPoint::CardStart] + self.root_offset()
    }

    pub fn special_button_point(&self) -> Vec2 {
        self.points[UiPoint::SpecialButton] + self.root_offset()
    }

    pub fn calculate_icon_position(start: Vec2, col: i32, row: i32) -> Vec2 {
        const HORIZONTAL_DIFF: f32 = 16.0;
        const VERTICAL_DIFF: f32 = 24.0;

        Vec2::new(
            start.x + col as f32 * HORIZONTAL_DIFF,
            start.y + row as f32 * VERTICAL_DIFF,
        )
    }

    pub fn advance_time(&mut self) {
        self.time += 1;
    }

    pub fn animate_form_list(
        &mut self,
        simulation: &mut BattleSimulation,
        selection: &mut CardSelectSelection,
    ) {
        let form_open_time = selection.form_open_time;
        let form_list_node = &mut self.sprites[self.form_list_index];

        // make visible if there's forms available
        if let Ok(player) = simulation
            .entities
            .query_one_mut::<&Player>(simulation.local_player_id.into())
        {
            if player.available_forms().next().is_some() {
                form_list_node.set_visible(true);
            } else {
                form_list_node.set_visible(false);
                selection.form_open_time = None;
                // no need to animate
                return;
            }
        };

        let Some(time) = form_open_time else {
            self.animator.set_state("FORM_TAB");
            form_list_node.apply_animation(&self.animator);
            return;
        };

        self.animator.set_state("FORM_LIST_FRAME");
        form_list_node.apply_animation(&self.animator);

        // animate origin
        let inital_origin = self.animator.point_or_zero("INITIAL_ORIGIN");
        let progress =
            inverse_lerp!(time, time + Self::FORM_LIST_ANIMATION_TIME, self.time).clamp(0.0, 1.0);
        let origin = inital_origin.lerp(self.animator.origin(), progress);
        form_list_node.set_origin(origin)
    }

    pub fn slide_progress(&self, confirm_time: FrameTime) -> f32 {
        let progress = if confirm_time == 0 {
            inverse_lerp!(0.0, Self::SLIDE_DURATION, self.time)
        } else {
            1.0 - inverse_lerp!(0.0, Self::SLIDE_DURATION, self.time - confirm_time)
        };

        progress.clamp(0.0, 1.0)
    }

    pub fn animate_slide(
        &mut self,
        simulation: &mut BattleSimulation,
        selection: &CardSelectSelection,
    ) {
        let progress = self.slide_progress(selection.confirm_time);

        let root_node = self.sprites.root_mut();
        let width = root_node.size().x;

        root_node.set_offset(Vec2::new((1.0 - progress) * -width, 0.0));

        // update health ui position
        let mut health_position = Vec2::new(BATTLE_UI_MARGIN, 0.0);

        if selection.visible {
            health_position.x += progress * width;
        }

        // note: moving health ui also moves emotion ui
        simulation.local_health_ui.set_position(health_position);
    }

    pub fn update_card_frame(&mut self, game_io: &GameIO, hand: &PlayerHand, card_index: usize) {
        let globals = Globals::from_resources(game_io);
        let (card, namespace) = &hand.deck[card_index];

        let card_packages = &globals.card_packages;
        let package = card_packages.package_or_fallback(*namespace, &card.package_id);

        let card_class = package
            .map(|package| package.card_properties.card_class)
            .unwrap_or_default();

        let state = match card_class {
            CardClass::Standard => "STANDARD_FRAME",
            CardClass::Mega => "MEGA_FRAME",
            CardClass::Giga => "GIGA_FRAME",
            CardClass::Dark => "DARK_FRAME",
            CardClass::Recipe => "STANDARD_FRAME",
        };

        let card_frame_node = &mut self.sprites[self.card_frame_index];
        self.animator.set_state(state);
        card_frame_node.apply_animation(&self.animator);
    }

    pub fn reset_card_frame(&mut self) {
        let card_frame_node = &mut self.sprites[self.card_frame_index];

        self.animator.set_state("STANDARD_FRAME");
        card_frame_node.apply_animation(&self.animator);
    }

    pub fn draw_tree(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.sprites.draw(sprite_queue)
    }

    pub fn draw_names(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        let mut background_sprite = self.recycled_sprite.clone();
        let edge_sprite = &mut self.recycled_sprite;

        self.animator.set_state("NAME_PLATE");
        self.animator.apply(&mut background_sprite);

        self.animator.set_state("NAME_PLATE_EDGE");
        self.animator.apply(edge_sprite);

        let mut style = TextStyle::new(game_io, FontName::Thick);
        let mut y = 3.0;

        // draw names
        for (_, (entity, character, name)) in simulation
            .entities
            .query_mut::<(&Entity, &Character, &EntityName)>()
        {
            if entity.team == simulation.local_team {
                continue;
            }

            let name = &name.0;
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
            sprite_queue.draw_sprite(edge_sprite);

            // draw name on top of plate
            style.draw(game_io, sprite_queue, name);

            // draw suffix after name
            style.bounds.x += name_width;
            style.draw(game_io, sprite_queue, suffix);

            y += 16.0;
        }
    }

    pub fn draw_staged_icons(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        hand: &PlayerHand,
    ) {
        const ITEM_OFFSET: Vec2 = Vec2::new(0.0, 16.0);

        let mut start = self.points[UiPoint::StagedCardStart];
        start.x += self.sprites.root().offset().x;

        // draw background frames
        self.animator.set_state("SELECTED_FRAME");
        self.animator.apply(&mut self.recycled_sprite);

        let mut position = start;

        for _ in 0..hand.staged_items.visible_count() {
            self.recycled_sprite.set_position(position);
            sprite_queue.draw_sprite(&self.recycled_sprite);
            position += ITEM_OFFSET;
        }

        // draw icons
        hand.staged_items
            .draw_icons(game_io, sprite_queue, &hand.deck, start, ITEM_OFFSET);
    }

    pub fn draw_confirm_preview(&mut self, hand: &PlayerHand, sprite_queue: &mut SpriteColorQueue) {
        if hand.staged_items.visible_count() > 0 {
            self.animator.set_state("CONFIRM_MESSAGE");
        } else {
            self.animator.set_state("EMPTY_MESSAGE");
        }

        let preview_point = self.preview_point();

        self.animator.apply(&mut self.recycled_sprite);
        self.recycled_sprite.set_position(preview_point);
        sprite_queue.draw_sprite(&self.recycled_sprite);
    }

    pub fn draw_cards_in_hand(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        player: &Player,
        hand: &PlayerHand,
    ) {
        self.animator.set_state("ICON_FRAME");
        self.animator.apply(&mut self.recycled_sprite);
        let maxed_card_usage = hand.staged_items.visible_count() >= 5;
        let card_restriction = CardSelectRestriction::resolve(hand);

        for (i, namespace, card, position) in self.card_icon_render_iter(player, hand) {
            sprite_queue.set_shader_effect(SpriteShaderEffect::Default);

            self.recycled_sprite.set_position(position);
            sprite_queue.draw_sprite(&self.recycled_sprite);

            if hand.staged_items.has_deck_index(i) {
                continue;
            }

            if maxed_card_usage || !card_restriction.allows_card(card) {
                sprite_queue.set_shader_effect(SpriteShaderEffect::Grayscale);
            } else {
                sprite_queue.set_shader_effect(SpriteShaderEffect::Default);
            }

            CardPackage::draw_icon(game_io, sprite_queue, namespace, &card.package_id, position);
        }

        sprite_queue.set_shader_effect(SpriteShaderEffect::Default);

        self.draw_card_codes_in_hand(game_io, sprite_queue, player, hand);
    }

    fn draw_card_codes_in_hand(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        player: &Player,
        hand: &PlayerHand,
    ) {
        const CODE_HORIZONTAL_OFFSET: f32 = 4.0;
        const CODE_VERTICAL_OFFSET: f32 = 16.0;

        let mut code_style = TextStyle::new(game_io, FontName::Code);
        code_style.color = Color::YELLOW;

        for (_, _, card, position) in self.card_icon_render_iter(player, hand) {
            code_style.bounds.set_position(position);

            code_style.bounds.x += CODE_HORIZONTAL_OFFSET;
            code_style.bounds.y += CODE_VERTICAL_OFFSET;

            code_style.draw(game_io, sprite_queue, &card.code);
        }
    }

    fn card_icon_render_iter<'a>(
        &self,
        player: &'a Player,
        hand: &'a PlayerHand,
    ) -> impl Iterator<Item = (usize, PackageNamespace, &'a Card, Vec2)> + use<'a> {
        let start = self.card_start_point();

        // draw icons
        hand.deck
            .iter()
            .take(player.hand_size())
            .enumerate()
            .map(move |(i, (card, namespace))| {
                let col = i % CARD_COLS;
                let row = i / CARD_COLS;

                let top_left = CardSelectUi::calculate_icon_position(start, col as i32, row as i32);

                (i, *namespace, card, top_left)
            })
    }

    pub fn draw_regular_card_frame(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.animator.set_state("REGULAR_FRAME");
        self.animator.set_loop_mode(AnimatorLoopMode::Loop);
        self.animator.sync_time(self.time);
        self.animator.apply(&mut self.recycled_sprite);

        let root_offset = self.sprites.root().offset();
        self.recycled_sprite.set_position(root_offset);
        sprite_queue.draw_sprite(&self.recycled_sprite);
    }

    pub fn draw_card_cursor(&mut self, sprite_queue: &mut SpriteColorQueue, col: i32, row: i32) {
        let position = Self::calculate_icon_position(self.card_start_point(), col, row);

        self.draw_state_at(sprite_queue, "CARD_CURSOR", position);
    }

    pub fn draw_confirm_cursor(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.draw_state_at(
            sprite_queue,
            "CONFIRM_CURSOR",
            self.points[UiPoint::Confirm] + self.root_offset(),
        );
    }

    pub fn draw_special_cursor(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.draw_state_at(
            sprite_queue,
            "SPECIAL_CURSOR",
            self.points[UiPoint::SpecialButton] + self.root_offset(),
        );
    }

    pub fn draw_card_button_cursor(
        &mut self,
        sprite_queue: &mut SpriteColorQueue,
        button_width: i32,
        col: i32,
        row: i32,
    ) {
        let card_start = self.card_start_point();

        let start_position = Self::calculate_icon_position(card_start, col, row);
        let end_position = Self::calculate_icon_position(card_start, col + button_width, row);

        self.draw_state_at(sprite_queue, "CARD_BUTTON_LEFT_CURSOR", start_position);
        self.draw_state_at(sprite_queue, "CARD_BUTTON_RIGHT_CURSOR", end_position);
    }

    pub fn draw_form_list(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        player: &Player,
        hand: &PlayerHand,
        selected_row: Option<usize>,
    ) {
        // draw frames
        self.animator.set_state("FORM_FRAME");
        self.animator.apply(&mut self.recycled_sprite);

        let row_height = self.recycled_sprite.bounds().height;
        let mut offset = self.points[UiPoint::FormListStart];

        for _ in player.available_forms() {
            self.recycled_sprite.set_position(offset);
            sprite_queue.draw_sprite(&self.recycled_sprite);

            offset.y += row_height;
        }

        // draw mugs
        let mut mug_sprite = Sprite::new(game_io, self.recycled_sprite.texture().clone());

        let mut offset = self.points[UiPoint::FormListStart];

        for (row, (index, form)) in player.available_forms().enumerate() {
            // default to grayscale
            sprite_queue.set_shader_effect(SpriteShaderEffect::Grayscale);

            if hand.staged_items.stored_form_index() == Some(index) {
                // selected color
                mug_sprite.set_color(Color::new(0.16, 1.0, 0.87, 1.0));
            } else if selected_row == Some(row) {
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
    }

    pub fn draw_form_cursor(&mut self, sprite_queue: &mut SpriteColorQueue, row: usize) {
        self.animator.set_state("FORM_FRAME");
        let row_height = self
            .animator
            .frame(0)
            .map(|frame| frame.size().y)
            .unwrap_or_default();

        let mut offset = self.points[UiPoint::FormListStart];
        offset.y += row_height * row as f32;

        self.animator.set_state("FORM_CURSOR");
        self.animator.set_loop_mode(AnimatorLoopMode::Loop);
        self.animator.sync_time(self.time);

        self.animator.apply(&mut self.recycled_sprite);
        self.recycled_sprite.set_position(offset);
        sprite_queue.draw_sprite(&self.recycled_sprite);
    }

    pub fn draw_final_turn_indicator(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.draw_state_at(sprite_queue, "FINAL_TURN", Vec2::ZERO);
    }

    fn draw_state_at(&mut self, sprite_queue: &mut SpriteColorQueue, state: &str, position: Vec2) {
        self.animator.set_state(state);
        self.animator.set_loop_mode(AnimatorLoopMode::Loop);
        self.animator.sync_time(self.time);
        self.animator.apply(&mut self.recycled_sprite);

        self.recycled_sprite.set_position(position);
        sprite_queue.draw_sprite(&self.recycled_sprite);
    }
}
