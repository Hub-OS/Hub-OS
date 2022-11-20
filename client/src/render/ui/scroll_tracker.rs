// could be split up into ScrollTracker, Scrollbar, and Cursor
// but keeping it all in one class should help reduce the amount of variables in each scene

use super::UiInputTracker;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::ops::Range;

pub struct ScrollTracker {
    view_size: usize,
    total_items: usize,
    selected_index: usize,
    top_index: usize,
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
    pub fn new(game_io: &GameIO<Globals>, view_size: usize) -> Self {
        let globals = game_io.globals();
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
            total_items: 0,
            selected_index: 0,
            top_index: 0,
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

    pub fn set_idle(&mut self, idle: bool) {
        let state = if idle { "IDLE" } else { "DEFAULT" };

        if self.cursor_animator.current_state() != Some(state) {
            self.cursor_animator.set_state(state);
            self.cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);
        }
    }

    pub fn use_custom_cursor(
        &mut self,
        game_io: &GameIO<Globals>,
        animation_path: &str,
        texture_path: &str,
    ) {
        let globals = game_io.globals();
        let assets = &globals.assets;

        let mut cursor_animator = Animator::load_new(assets, animation_path);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        self.cursor_animator = cursor_animator.clone();

        cursor_animator.set_state("SELECTED");
        self.remembered_animator = cursor_animator;

        self.cursor_sprite
            .set_texture(assets.texture(game_io, texture_path));
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

    pub fn view_size(&self) -> usize {
        self.view_size
    }

    pub fn view_range(&self) -> Range<usize> {
        Range {
            start: self.top_index,
            end: (self.top_index + self.view_size).min(self.total_items),
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

    pub fn remembered_index(&self) -> Option<usize> {
        self.remembered_index
    }

    pub fn remember_index(&mut self) {
        self.remembered_index = Some(self.selected_index)
    }

    pub fn forget_index(&mut self) -> Option<usize> {
        self.remembered_index.take()
    }

    pub fn handle_vertical_input(&mut self, ui_input_tracker: &UiInputTracker) {
        if ui_input_tracker.is_active(Input::Up) {
            self.move_up();
        }

        if ui_input_tracker.is_active(Input::Down) {
            self.move_down();
        }

        if ui_input_tracker.is_active(Input::ShoulderL) {
            self.page_up();
        }

        if ui_input_tracker.is_active(Input::ShoulderR) {
            self.page_down();
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
        self.set_selected_index(self.selected_index.max(1) - 1);
    }

    pub fn move_down(&mut self) {
        self.set_selected_index(self.selected_index + 1);
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
        if let Some(index) = self.remembered_index {
            if self.view_range().contains(&index) {
                self.internal_draw_cursor(sprite_queue, false, index);
            }
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

        let mut position = self.cursor_start;

        let offset = (index - self.top_index) as f32 * self.cursor_multiplier;

        if self.vertical {
            position.y += offset;
        } else {
            position.x += offset;
        }

        self.cursor_sprite.set_position(position);
        sprite_queue.draw_sprite(&self.cursor_sprite);
    }
}
