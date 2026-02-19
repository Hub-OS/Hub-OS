// could be split up into ScrollTracker, Scrollbar, and Cursor
// but keeping it all in one class should help reduce the amount of variables in each scene

use super::UiInputTracker;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::ops::Range;

pub struct ScrollTracker {
    view_size: usize,
    view_margin: usize,
    total_items: usize,
    selected_index: usize,
    top_index: usize,
    wrap: bool,
    scroll_start: Vec2,
    scroll_end: Vec2,
    scroll_thumb_sprite: Sprite,
    cursor_start: Vec2,
    cursor_multiplier: f32,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    remembered_index: Option<usize>,
    remembered_animator: Animator,
    vertical: bool,
}

impl ScrollTracker {
    pub fn new(game_io: &GameIO, view_size: usize) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        // scroller
        let mut scroll_thumb_sprite = assets.new_sprite(game_io, ResourcePaths::SCROLLBAR_THUMB);
        let thumb_origin = scroll_thumb_sprite.size() * 0.5;
        scroll_thumb_sprite.set_origin(thumb_origin);

        // cursor
        let cursor_sprite = assets.new_sprite(game_io, ResourcePaths::SELECT_CURSOR);

        let mut cursor_animator =
            Animator::load_new(assets, ResourcePaths::SELECT_CURSOR_ANIMATION);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        let mut remembered_animator = cursor_animator.clone();
        remembered_animator.set_state("SELECTED");
        remembered_animator.set_loop_mode(AnimatorLoopMode::Loop);

        Self {
            view_size,
            view_margin: 0,
            total_items: 0,
            selected_index: 0,
            top_index: 0,
            wrap: false,
            scroll_start: Vec2::ZERO,
            scroll_end: Vec2::ZERO,
            scroll_thumb_sprite,
            cursor_start: Vec2::ZERO,
            cursor_multiplier: 0.0,
            cursor_sprite,
            cursor_animator,
            remembered_index: None,
            remembered_animator,
            vertical: true,
        }
    }

    pub fn with_view_margin(mut self, view_margin: usize) -> Self {
        self.view_margin = view_margin;
        self
    }

    pub fn with_wrap(mut self, wrap: bool) -> Self {
        self.wrap = wrap;
        self
    }

    pub fn with_total_items(mut self, total_items: usize) -> Self {
        self.set_total_items(total_items);
        self
    }

    pub fn with_selected_index(mut self, index: usize) -> Self {
        self.set_selected_index(index);
        self
    }

    pub fn with_custom_cursor(
        mut self,
        game_io: &GameIO,
        animation_path: &str,
        texture_path: &str,
    ) -> Self {
        self.use_custom_cursor(game_io, animation_path, texture_path);
        self
    }

    pub fn with_custom_cursor_states(mut self, default: &str, selected: &str) -> Self {
        self.set_custom_cursor_states(default, selected);
        self
    }

    pub fn set_idle(&mut self, idle: bool) {
        let state = if idle { "IDLE" } else { "DEFAULT" };

        if self.cursor_animator.current_state() != Some(state) {
            self.cursor_animator.set_state(state);
            self.cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);
        }
    }

    pub fn use_custom_cursor(
        &mut self,
        game_io: &GameIO,
        animation_path: &str,
        texture_path: &str,
    ) {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        self.cursor_animator = Animator::load_new(assets, animation_path);
        self.remembered_animator = self.cursor_animator.clone();

        self.cursor_sprite
            .set_texture(assets.texture(game_io, texture_path));

        self.set_custom_cursor_states("DEFAULT", "SELECTED");
    }

    pub fn set_custom_cursor_states(&mut self, default: &str, selected: &str) {
        self.cursor_animator.set_state(default);
        self.cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        self.remembered_animator.set_state(selected);
        self.remembered_animator
            .set_loop_mode(AnimatorLoopMode::Loop);
    }

    pub fn selected_cursor_position(&self) -> Vec2 {
        self.internal_cursor_position(self.selected_index)
    }

    pub fn internal_cursor_position(&self, index: usize) -> Vec2 {
        let mut position = self.cursor_start;

        let offset = (index - self.top_index) as f32 * self.cursor_multiplier;

        if self.vertical {
            position.y += offset;
        } else {
            position.x += offset;
        }

        position
    }

    pub fn cursor_definition(&self) -> (Vec2, f32) {
        (self.cursor_start, self.cursor_multiplier)
    }

    pub fn define_cursor(&mut self, start: Vec2, multiplier: f32) {
        self.cursor_start = start;
        self.cursor_multiplier = multiplier;
    }

    pub fn set_vertical(&mut self, vertical: bool) {
        self.vertical = vertical;
    }

    pub fn cursor_multiplier(&self) -> f32 {
        self.cursor_multiplier
    }

    pub fn scrollbar_definition(&self) -> (Vec2, Vec2) {
        (self.scroll_start, self.scroll_end)
    }

    pub fn define_scrollbar(&mut self, start: Vec2, end: Vec2) {
        self.scroll_start = start;
        self.scroll_end = end;
    }

    pub fn top_index(&self) -> usize {
        self.top_index
    }

    pub fn selected_index(&self) -> usize {
        self.selected_index
    }

    pub fn total_items(&self) -> usize {
        self.total_items
    }

    pub fn view_margin(&self) -> usize {
        self.view_margin
    }

    pub fn view_size(&self) -> usize {
        self.view_size
    }

    pub fn set_view_size(&mut self, view_size: usize) {
        self.view_size = view_size;

        // refresh selected_index to recalculate top_index
        self.set_selected_index(self.selected_index);
    }

    pub fn view_range(&self) -> Range<usize> {
        Range {
            start: self.top_index.max(self.view_margin) - self.view_margin,
            end: (self.top_index + self.view_size + self.view_margin).min(self.total_items),
        }
    }

    pub fn progress(&self) -> f32 {
        if self.total_items <= self.view_size {
            return 0.0;
        }

        self.top_index as f32 / (self.total_items - self.view_size) as f32
    }

    pub fn is_in_view(&self, index: usize) -> bool {
        index >= self.top_index && index < self.top_index + self.view_size
    }

    pub fn set_total_items(&mut self, count: usize) {
        self.total_items = count;

        if self.selected_index + 1 > self.total_items {
            self.selected_index = self.total_items.max(1) - 1;
        }

        if self.top_index + self.view_size >= self.total_items {
            self.top_index = self.total_items.max(self.view_size) - self.view_size;
        }

        self.forget_index();
    }

    pub fn set_selected_index(&mut self, index: usize) {
        self.selected_index = index.min(self.total_items.max(1) - 1);

        if self.top_index > self.selected_index {
            self.top_index = self.selected_index;
        } else if self.top_index + self.view_size <= self.selected_index {
            self.top_index = self.selected_index.max(self.view_size) - self.view_size + 1;
        }
    }

    pub fn set_top_index(&mut self, index: usize) {
        let relative_index = self.selected_index - self.top_index;
        self.top_index = index.min(self.total_items.max(self.view_size) - self.view_size);

        self.selected_index = (self.top_index + relative_index).min(self.total_items.max(1) - 1);
    }

    /// Shifts selected index, remembered index, top index, and updates total items.
    pub fn handle_insert(&mut self, index: usize, len: usize) {
        if index <= self.selected_index {
            self.selected_index += len;
        }

        if let Some(remembered_index) = &mut self.remembered_index
            && index <= *remembered_index
        {
            *remembered_index += len;
        }

        if index <= self.top_index {
            self.top_index += len;
        }

        self.total_items += len;

        // keep selected index in view
        self.set_selected_index(self.selected_index);
    }

    /// Shifts selected index, remembered index, top index, and updates total items.
    pub fn handle_remove(&mut self, index: usize) {
        if index < self.selected_index {
            self.selected_index -= 1;
        }

        if let Some(remembered_index) = &mut self.remembered_index
            && index < *remembered_index
        {
            *remembered_index -= 1;
        }

        if index < self.top_index {
            self.top_index -= 1;
        }

        self.total_items -= 1;
    }

    pub fn remembered_index(&self) -> Option<usize> {
        self.remembered_index
    }

    pub fn remember_index(&mut self) {
        if self.total_items <= self.selected_index {
            log::warn!("Attempted to remember an out of bounds index");
            return;
        }
        self.remembered_index = Some(self.selected_index)
    }

    pub fn set_remembered_index(&mut self, index: usize) {
        self.remembered_index = Some(index)
    }

    pub fn forget_index(&mut self) -> Option<usize> {
        self.remembered_index.take()
    }

    pub fn handle_page_input(&mut self, ui_input_tracker: &UiInputTracker) {
        if ui_input_tracker.pulsed(Input::ShoulderL) {
            self.page_up();
        }

        if ui_input_tracker.pulsed(Input::ShoulderR) {
            self.page_down();
        }
    }

    pub fn handle_vertical_input(&mut self, ui_input_tracker: &UiInputTracker) {
        if ui_input_tracker.pulsed(Input::Up) {
            self.move_up();
        }

        if ui_input_tracker.pulsed(Input::Down) {
            self.move_down();
        }

        self.handle_page_input(ui_input_tracker);
    }

    pub fn handle_horizontal_input(&mut self, ui_input_tracker: &UiInputTracker) {
        if ui_input_tracker.pulsed(Input::Left) {
            self.move_up();
        }

        if ui_input_tracker.pulsed(Input::Right) {
            self.move_down();
        }
    }

    pub fn page_up(&mut self) {
        let relative_index = self.selected_index - self.top_index;

        if self.top_index < self.view_size {
            // can't move up by a whole view
            self.top_index = 0;
        } else {
            // move up by a whole view
            self.top_index -= self.view_size;
        }

        // update selected_index
        self.selected_index = self.top_index + relative_index;
    }

    pub fn page_down(&mut self) {
        let relative_index = self.selected_index - self.top_index;

        // move down by a whole view
        self.top_index += self.view_size;

        // keep in range
        if self.top_index + self.view_size >= self.total_items {
            self.top_index = self.total_items.max(self.view_size) - self.view_size;
        }

        // update selected_index
        self.selected_index = self.top_index + relative_index;
    }

    pub fn move_up(&mut self) {
        let new_index = if self.selected_index > 0 {
            self.selected_index - 1
        } else if self.wrap {
            self.total_items.max(1) - 1
        } else {
            0
        };

        self.set_selected_index(new_index);
    }

    pub fn move_down(&mut self) {
        let mut new_index = self.selected_index + 1;

        if self.wrap {
            new_index = new_index.checked_rem(self.total_items).unwrap_or_default();
        }

        self.set_selected_index(new_index);
    }

    pub fn move_view_up(&mut self) {
        self.set_top_index(self.top_index.max(1) - 1);
    }

    pub fn move_view_down(&mut self) {
        self.set_top_index(self.top_index + 1);
    }

    pub fn draw_scrollbar(&mut self, sprite_queue: &mut SpriteColorQueue) {
        let position = self.scroll_start.lerp(self.scroll_end, self.progress());

        self.scroll_thumb_sprite.set_position(position);
        sprite_queue.draw_sprite(&self.scroll_thumb_sprite)
    }

    pub fn draw_cursor(&mut self, sprite_queue: &mut SpriteColorQueue) {
        // draw selected index
        self.internal_draw_cursor(sprite_queue, true, self.selected_index);

        // draw remembered_index
        if let Some(index) = self.remembered_index
            && self.view_range().contains(&index)
        {
            self.internal_draw_cursor(sprite_queue, false, index);
        }
    }

    fn internal_draw_cursor(
        &mut self,
        sprite_queue: &mut SpriteColorQueue,
        use_default_animator: bool,
        index: usize,
    ) {
        let animator = match use_default_animator {
            true => &mut self.cursor_animator,
            false => &mut self.remembered_animator,
        };

        animator.update();
        animator.apply(&mut self.cursor_sprite);

        let position = self.internal_cursor_position(index);
        self.cursor_sprite.set_position(position);
        sprite_queue.draw_sprite(&self.cursor_sprite);
    }
}
