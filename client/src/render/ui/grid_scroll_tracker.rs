use super::{ScrollTracker, UiInputTracker};
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
    pub fn new(game_io: &GameIO, width: usize, height: usize) -> Self {
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

    pub fn set_total_items(&mut self, total_items: usize) {
        self.total_items = total_items;

        let mut main_size = total_items / self.columns;

        if total_items % self.columns > 0 {
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
}
