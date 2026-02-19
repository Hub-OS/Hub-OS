use super::{ScrollTracker, UiInputTracker};
use crate::render::SpriteColorQueue;
use framework::prelude::*;

pub struct GridScrollTracker {
    total_items: usize,
    columns: usize,
    h_scroll_tracker: ScrollTracker,
    v_scroll_tracker: ScrollTracker,
    position: Vec2,
    step: Vec2,
}

impl GridScrollTracker {
    pub fn new(game_io: &GameIO, mut width: usize, mut height: usize) -> Self {
        width = width.max(1);
        height = height.max(1);

        Self {
            total_items: 0,
            columns: width,
            h_scroll_tracker: ScrollTracker::new(game_io, width).with_wrap(true),
            v_scroll_tracker: ScrollTracker::new(game_io, height),
            position: Vec2::ZERO,
            step: Vec2::ZERO,
        }
    }

    pub fn with_position(mut self, position: Vec2) -> Self {
        self.position = position;
        self
    }

    pub fn with_step(mut self, step: Vec2) -> Self {
        self.step = step;
        self
    }

    pub fn with_view_margin(mut self, view_margin: usize) -> Self {
        self.v_scroll_tracker = self.v_scroll_tracker.with_view_margin(view_margin);
        self
    }

    pub fn with_total_items(mut self, total_items: usize) -> Self {
        self.set_total_items(total_items);
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

    pub fn use_custom_cursor(
        &mut self,
        game_io: &GameIO,
        animation_path: &str,
        texture_path: &str,
    ) {
        self.v_scroll_tracker
            .use_custom_cursor(game_io, animation_path, texture_path);
    }

    pub fn set_custom_cursor_states(&mut self, default: &str, selected: &str) {
        self.v_scroll_tracker
            .set_custom_cursor_states(default, selected);
    }

    pub fn set_view_size(&mut self, width: usize, height: usize) {
        self.h_scroll_tracker.set_view_size(width.max(1));
        self.v_scroll_tracker.set_view_size(height.max(1));
        self.columns = width;
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.position = position;
    }

    pub fn set_step(&mut self, step: Vec2) {
        self.step = step;
    }

    pub fn set_total_items(&mut self, total_items: usize) {
        self.total_items = total_items;

        let mut main_size = total_items / self.columns;

        if !total_items.is_multiple_of(self.columns) {
            main_size += 1;
        }

        self.v_scroll_tracker.set_total_items(main_size);

        let cross_size = self.row_size_at(self.v_scroll_tracker.selected_index());
        self.h_scroll_tracker.set_total_items(cross_size);
    }

    fn row_size_at(&self, row: usize) -> usize {
        if row + 1 < self.v_scroll_tracker.total_items() {
            // not at the last row, max size
            self.columns
        } else {
            // at the last row, use the remainder to resolve the size
            let remaining_items = self.total_items % self.columns;

            if remaining_items == 0 {
                // perfect division, max size
                self.columns
            } else {
                remaining_items
            }
        }
    }

    pub fn selected_index(&self) -> usize {
        let v_index = self.v_scroll_tracker.selected_index();
        let h_index = self.h_scroll_tracker.selected_index();

        v_index * self.columns + h_index
    }

    pub fn set_selected_index(&mut self, index: usize) {
        let v_index = index / self.columns;
        let h_index = index % self.columns;

        self.v_scroll_tracker.set_selected_index(v_index);

        let cross_size = self.row_size_at(self.v_scroll_tracker.selected_index());
        self.h_scroll_tracker.set_total_items(cross_size);

        self.h_scroll_tracker.set_selected_index(h_index);
    }

    pub fn handle_input(&mut self, input_tracker: &UiInputTracker) {
        self.v_scroll_tracker.handle_vertical_input(input_tracker);

        let cross_size = self.row_size_at(self.v_scroll_tracker.selected_index());
        self.h_scroll_tracker.set_total_items(cross_size);

        self.h_scroll_tracker.handle_horizontal_input(input_tracker);
    }

    pub fn index_position(&self, index: usize) -> Vec2 {
        let view_range = self.v_scroll_tracker.view_range();
        let row = index / self.columns;
        let col = index % self.columns;

        let mut position = self.position;
        position.x += self.step.x * col as f32;
        position.y += self.step.y * (row - view_range.start) as f32;

        // account for margin
        let margin = self.v_scroll_tracker.view_margin();
        let top_index = self.v_scroll_tracker.top_index();
        position.y -= self.step.y * margin.min(top_index) as f32;

        position
    }

    pub fn iter_visible(&self) -> impl Iterator<Item = (usize, Vec2)> + '_ {
        let v_range = self.v_scroll_tracker.view_range();
        let mut position = self.position;

        let margin = self.v_scroll_tracker.view_margin();
        let top_index = self.v_scroll_tracker.top_index();

        position.y -= self.step.y * margin.min(top_index) as f32;

        v_range.into_iter().flat_map(move |row| {
            let mut row_next = position;
            position.y += self.step.y;

            (0..self.row_size_at(row)).map(move |col| {
                let item_position = row_next;
                row_next.x += self.step.x;

                (row * self.columns + col, item_position)
            })
        })
    }

    pub fn define_cursor(&mut self, offset: Vec2) {
        // store definition in h_scroll_tracker
        self.h_scroll_tracker.define_cursor(offset, 0.0);
    }

    pub fn draw_cursor(&mut self, sprite_queue: &mut SpriteColorQueue) {
        // read definition from h_scroll_tracker and render using v_scroll_tracker
        let (offset, _) = self.h_scroll_tracker.cursor_definition();

        let h_index = self.h_scroll_tracker.selected_index();
        let v_index = self
            .v_scroll_tracker
            .selected_index()
            .saturating_sub(self.v_scroll_tracker.top_index());

        let position =
            self.position + offset + Vec2::new(h_index as f32, v_index as f32) * self.step;

        self.v_scroll_tracker.define_cursor(position, 0.0);
        self.v_scroll_tracker.draw_cursor(sprite_queue);
    }
}
