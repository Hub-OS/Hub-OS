use super::{ScrollTracker, ScrollableFrame, UiInputTracker, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::Globals;
use framework::prelude::{GameIO, Rect, Vec2};

pub struct ScrollableList {
    frame: ScrollableFrame,
    scroll_tracker: ScrollTracker,
    children: Vec<Box<dyn UiNode>>,
    focused: bool,
}

impl ScrollableList {
    pub fn new(game_io: &GameIO<Globals>, bounds: Rect, item_height: f32) -> Self {
        let frame = ScrollableFrame::new(game_io, bounds);
        let inner_bounds = frame.body_bounds();

        let mut scroll_tracker =
            ScrollTracker::new(game_io, (inner_bounds.height / item_height) as usize);
        scroll_tracker.define_scrollbar(frame.scroll_start(), frame.scroll_end());
        scroll_tracker.define_cursor(inner_bounds.top_left() + Vec2::new(-7.0, 0.0), item_height);

        Self {
            frame,
            scroll_tracker,
            children: Vec::new(),
            focused: true,
        }
    }

    pub fn with_label_str(mut self, label: &str) -> Self {
        self.frame.set_label(label.to_string());
        self
    }

    pub fn with_focus(mut self, focused: bool) -> Self {
        self.focused = focused;
        self
    }

    pub fn with_children(mut self, children: Vec<Box<dyn UiNode>>) -> Self {
        self.scroll_tracker.set_total_items(children.len());
        self.children = children;
        self
    }

    pub fn selected_index(&self) -> usize {
        self.scroll_tracker.selected_index()
    }

    pub fn bounds(&self) -> Rect {
        self.frame.bounds()
    }

    pub fn set_bounds(&mut self, bounds: Rect) {
        self.frame.update_bounds(bounds);

        let inner_bounds = self.frame.body_bounds();
        let item_height = self.scroll_tracker.cursor_multiplier();

        self.scroll_tracker
            .set_view_size((inner_bounds.height / item_height) as usize);
        self.scroll_tracker
            .define_scrollbar(self.frame.scroll_start(), self.frame.scroll_end());
        self.scroll_tracker
            .define_cursor(inner_bounds.top_left() + Vec2::new(-7.0, 0.0), item_height);
    }

    pub fn set_label(&mut self, label: String) {
        self.frame.set_label(label);
    }

    pub fn set_children(&mut self, children: Vec<Box<dyn UiNode>>) {
        self.scroll_tracker.set_total_items(children.len());
        self.scroll_tracker.set_selected_index(0);
        self.children = children;
    }

    pub fn is_focus_locked(&self) -> bool {
        let index = self.scroll_tracker.selected_index();

        let Some(child) = self.children.get(index) else {
            return false;
        };

        child.is_locking_focus()
    }

    pub fn set_focused(&mut self, focused: bool) {
        self.focused = focused;
    }

    pub fn page_up(&mut self) {
        if self.is_focus_locked() {
            return;
        }

        self.scroll_tracker.page_up();
    }

    pub fn page_down(&mut self) {
        if self.is_focus_locked() {
            return;
        }

        self.scroll_tracker.page_down();
    }

    pub fn update(&mut self, game_io: &mut GameIO<Globals>, ui_input_tracker: &UiInputTracker) {
        // update selection
        if self.focused && !self.is_focus_locked() {
            let previous_index = self.scroll_tracker.selected_index();

            self.scroll_tracker.handle_vertical_input(ui_input_tracker);

            if self.scroll_tracker.selected_index() != previous_index {
                let globals = game_io.globals();
                globals.audio.play_sound(&globals.cursor_move_sfx);
            }
        }

        // update children
        let mut child_bounds = self.frame.body_bounds();
        child_bounds.height = self.scroll_tracker.cursor_multiplier();

        for i in self.scroll_tracker.view_range() {
            let child = &mut self.children[i];
            child.update(
                game_io,
                child_bounds,
                i == self.scroll_tracker.selected_index() && self.focused,
            );

            child_bounds.y += child_bounds.height;
        }
    }

    pub fn draw(&mut self, game_io: &GameIO<Globals>, sprite_queue: &mut SpriteColorQueue) {
        // draw frame
        self.frame.draw(game_io, sprite_queue);

        // draw children
        let mut child_bounds = self.frame.body_bounds();
        child_bounds.height = self.scroll_tracker.cursor_multiplier();

        for i in self.scroll_tracker.view_range() {
            let child = &mut self.children[i];
            child.draw_bounded(game_io, sprite_queue, child_bounds);

            child_bounds.y += child_bounds.height;
        }

        // draw scrollbar + cursor
        self.scroll_tracker.draw_scrollbar(sprite_queue);

        if self.focused && !self.is_focus_locked() {
            self.scroll_tracker.draw_cursor(sprite_queue);
        }
    }
}
