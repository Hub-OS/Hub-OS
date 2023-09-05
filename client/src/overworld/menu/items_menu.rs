use super::Menu;
use crate::overworld::OverworldArea;
use crate::packages::PackageNamespace;
use crate::render::ui::{
    build_9patch, ElementSprite, FontStyle, NinePatch, PlayerHealthUi, SceneTitle, ScrollTracker,
    SubSceneFrame, Text, TextStyle, Textbox, UiInputTracker,
};
use crate::render::{Animator, AnimatorLoopMode, Background, SpriteColorQueue};
use crate::resources::{
    AssetManager, Globals, InputUtil, ResourcePaths, TEXT_DARK_SHADOW_COLOR,
    TEXT_TRANSPARENT_SHADOW_COLOR,
};
use framework::prelude::*;
use packets::structures::{Input, ItemDefinition};

pub struct ItemsMenu {
    background: Background,
    frame: SubSceneFrame,
    health_ui: PlayerHealthUi,
    ui_input_tracker: UiInputTracker,
    scroll_tracker: ScrollTracker,
    background_sprites: Vec<Sprite>,
    player_sprite: Sprite,
    player_animator: Animator,
    description_text: Text,
    button_nine_patch: NinePatch,
    items_start: Vec2,
    item_width: f32,
    registered_count: usize,
    is_open: bool,
    on_select: Box<dyn Fn(&str)>,
}

impl ItemsMenu {
    pub fn new(game_io: &GameIO, area: &OverworldArea, on_select: impl Fn(&str) + 'static) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // buttons nine patch
        let patch_texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let patch_animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);
        let button_nine_patch = build_9patch!(game_io, patch_texture, &patch_animator, "BUTTON");

        // layout
        let layout_animator =
            Animator::load_new(assets, ResourcePaths::ITEMS_LAYOUT_ANIMATION).with_state("DEFAULT");
        let health_position = layout_animator.point("HEALTH").unwrap_or_default();
        let element_position = layout_animator.point("ELEMENT").unwrap_or_default();
        let player_position = layout_animator.point("PLAYER").unwrap_or_default();
        let cursor_start = layout_animator.point("CURSOR_START").unwrap_or_default();
        let items_start = layout_animator.point("ITEMS_START").unwrap_or_default();
        let item_width = layout_animator.point("ITEM_SIZE").unwrap_or_default().x;
        let description_bg_start = layout_animator.point("DESCRIPTION_BG").unwrap_or_default();
        let description_start = layout_animator
            .point("DESCRIPTION_START")
            .unwrap_or_default();
        let description_end = layout_animator.point("DESCRIPTION_END").unwrap_or_default();

        // background sprites, items added after this
        let mut background_sprites = Vec::new();

        // health ui
        let mut health_ui = PlayerHealthUi::new(game_io);
        health_ui.snap_health(area.player_data.health);
        health_ui.set_position(health_position);

        // element
        let package_id = &area.player_data.package_id;
        let player_package = globals
            .player_packages
            .package_or_override(PackageNamespace::Local, package_id)
            .unwrap();

        let mut element_sprite = ElementSprite::new(game_io, player_package.element);
        element_sprite.set_position(element_position);
        background_sprites.push(element_sprite);

        // player
        let mut player_sprite = assets.new_sprite(game_io, &player_package.overworld_texture_path);
        let player_animator = Animator::load_new(assets, &player_package.overworld_animation_path)
            .with_state("IDLE_D")
            .with_loop_mode(AnimatorLoopMode::Loop);

        player_sprite.set_position(player_position);
        player_animator.apply(&mut player_sprite);

        // scroll tracker
        let mut scroll_tracker = ScrollTracker::new(game_io, 8);
        let button_height = TextStyle::new(game_io, FontStyle::Thick).line_height()
            + button_nine_patch.top_height()
            + button_nine_patch.bottom_height();
        scroll_tracker.define_cursor(cursor_start, button_height);

        // description bg sprite
        let mut description_bg_sprite = assets.new_sprite(game_io, ResourcePaths::ITEM_DESCRIPTION);
        description_bg_sprite.set_position(description_bg_start);
        background_sprites.push(description_bg_sprite);

        // description text
        let mut description_text = Text::new(game_io, FontStyle::Thin);
        description_text.style.shadow_color = TEXT_TRANSPARENT_SHADOW_COLOR;
        description_text.style.bounds = Rect::from_corners(description_start, description_end);
        description_text.text = String::from("You have no items.");

        Self {
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_left_arm(true)
                .with_right_arm(true),
            health_ui,
            ui_input_tracker: UiInputTracker::new(),
            scroll_tracker,
            background_sprites,
            player_sprite,
            player_animator,
            description_text,
            button_nine_patch,
            items_start,
            item_width,
            registered_count: 0,
            is_open: true,
            on_select: Box::new(on_select),
        }
    }

    fn consumables_iter(
        area: &OverworldArea,
    ) -> impl Iterator<Item = (&String, &ItemDefinition, usize)> {
        let item_iter = area.player_data.inventory.items();

        item_iter
            .flat_map(|(item_id, count)| Some((item_id, area.item_registry.get(item_id)?, *count)))
            .filter(|(_, item_definition, _)| item_definition.consumable)
    }

    fn update_item_count(&mut self, area: &OverworldArea) {
        let registered_count = area.item_registry.len();

        // we only need to update the count if a new item was registered
        if self.registered_count == registered_count {
            return;
        }

        // if there were previously 0 items, initialize the description text
        if self.registered_count == 0 {
            if let Some((_, item_definition, _)) = Self::consumables_iter(area).next() {
                self.description_text.text = item_definition.description.clone();
            }
        }

        self.registered_count = registered_count;

        // update the total items for the scroll tracker
        let total_items = Self::consumables_iter(area).count();
        self.scroll_tracker.set_total_items(total_items);
    }

    fn update_player_sprite(&mut self, area: &mut OverworldArea) {
        let entities = &mut area.entities;
        let player_entity = area.player_data.entity;
        let overworld_player_sprite = entities.query_one_mut::<&Sprite>(player_entity).unwrap();

        let rotation = overworld_player_sprite.rotation();
        let scale = overworld_player_sprite.scale();
        let color = overworld_player_sprite.color();

        self.player_sprite.set_rotation(rotation);
        self.player_sprite.set_scale(scale);
        self.player_sprite.set_color(color);

        self.player_animator.update();
        self.player_animator.apply(&mut self.player_sprite);
    }
}

impl Menu for ItemsMenu {
    fn is_fullscreen(&self) -> bool {
        true
    }

    fn drop_on_close(&self) -> bool {
        true
    }

    fn is_open(&self) -> bool {
        self.is_open
    }

    fn open(&mut self, _game_io: &mut GameIO, _area: &mut OverworldArea) {}

    fn update(&mut self, game_io: &mut GameIO, area: &mut OverworldArea) {
        self.background.update();
        self.ui_input_tracker.update(game_io);
        self.update_item_count(area);
        self.update_player_sprite(area);
        self.health_ui.set_health(area.player_data.health);
    }

    fn handle_input(
        &mut self,
        game_io: &mut GameIO,
        area: &mut OverworldArea,
        _textbox: &mut Textbox,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            self.is_open = false;
            return;
        }

        // handle scrolling
        let prev_index = self.scroll_tracker.selected_index();

        self.scroll_tracker
            .handle_vertical_input(&self.ui_input_tracker);

        let index = self.scroll_tracker.selected_index();

        if prev_index != index {
            globals.audio.play_sound(&globals.sfx.cursor_move);

            // update description
            let (_, item_definition, _) = Self::consumables_iter(area).nth(index).unwrap();
            self.description_text.text = item_definition.description.clone();
        }

        // handle selection
        if input_util.was_just_pressed(Input::Confirm) {
            if let Some((item_id, _, count)) = Self::consumables_iter(area).nth(index) {
                if count > 0 {
                    globals.audio.play_sound(&globals.sfx.cursor_select);
                    (self.on_select)(item_id);
                } else {
                    globals.audio.play_sound(&globals.sfx.cursor_error);
                }
            } else {
                globals.audio.play_sound(&globals.sfx.cursor_error);
            }
        }
    }

    fn draw(
        &mut self,
        game_io: &GameIO,
        render_pass: &mut RenderPass,
        sprite_queue: &mut SpriteColorQueue,
        area: &OverworldArea,
    ) {
        // draw background
        self.background.draw(game_io, render_pass);

        // draw player first, as the information is the important bit
        sprite_queue.draw_sprite(&self.player_sprite);

        // draw background sprites
        for sprite in &self.background_sprites {
            sprite_queue.draw_sprite(sprite);
        }

        // draw description
        self.description_text.draw(game_io, sprite_queue);

        // draw items
        let range = self.scroll_tracker.view_range();
        let consumable_iter = Self::consumables_iter(area)
            .skip(range.start)
            .take(range.end - range.start);

        let mut text_style =
            TextStyle::new(game_io, FontStyle::Thick).with_shadow_color(TEXT_DARK_SHADOW_COLOR);

        let mut patch_bounds = Rect::new(
            self.items_start.x,
            self.items_start.y,
            self.item_width,
            text_style.line_height(),
        );

        text_style.bounds.set_position(self.items_start);

        if self.scroll_tracker.total_items() > 0 {
            let step_y = patch_bounds.height
                + self.button_nine_patch.top_height()
                + self.button_nine_patch.bottom_height();

            for (_, item_definition, count) in consumable_iter {
                // draw item button
                self.button_nine_patch.draw(sprite_queue, patch_bounds);

                // create + render text
                let text = format!("{:8} {:>3}", item_definition.name, count);
                text_style.draw(game_io, sprite_queue, &text);

                // update bounds
                text_style.bounds.y += step_y;
                patch_bounds.y += step_y;
            }
        } else {
            // display a default item
            self.button_nine_patch.draw(sprite_queue, patch_bounds);
            text_style.draw(game_io, sprite_queue, "No Items");
        }

        // draw cursor
        self.scroll_tracker.draw_cursor(sprite_queue);

        // draw health
        self.health_ui.draw(game_io, sprite_queue);

        // draw frame
        self.frame.draw(sprite_queue);
        SceneTitle::new("ITEMS").draw(game_io, sprite_queue);
    }
}
