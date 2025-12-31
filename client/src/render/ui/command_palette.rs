use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct CommandPalette<T: Copy + 'static> {
    frame: ContextFrame,
    scroll_tracker: ScrollTracker,
    options: Vec<(String, T)>,
    width: f32,
    open: bool,
}

impl<T: Copy + 'static> CommandPalette<T> {
    pub fn new(game_io: &GameIO, label_translation_key: &str) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();

        Self {
            frame: ContextFrame::new(game_io, globals.translate(label_translation_key)),
            scroll_tracker: ScrollTracker::new(game_io, 6).with_wrap(true),
            options: Default::default(),
            width: RESOLUTION_F.x * 0.7,
            open: false,
        }
    }

    pub fn with_width(mut self, width: f32) -> Self {
        self.width = width;
        self
    }

    pub fn with_options(mut self, options: Vec<(String, T)>) -> Self {
        self.set_options(options);
        self
    }

    pub fn set_options(&mut self, options: Vec<(String, T)>) {
        self.scroll_tracker.set_total_items(options.len());
        self.options = options;
    }

    pub fn is_open(&self) -> bool {
        self.open
    }

    pub fn open(&mut self) {
        self.open = true;
        self.scroll_tracker.set_selected_index(0);
    }

    pub fn close(&mut self) {
        self.open = false;
    }

    pub fn update(&mut self, game_io: &mut GameIO, ui_input_tracker: &UiInputTracker) -> Option<T> {
        if !self.open {
            return None;
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            // closed
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            self.open = false;
            return None;
        }

        // scroll
        let prev_index = self.scroll_tracker.selected_index();

        self.scroll_tracker.handle_vertical_input(ui_input_tracker);

        if prev_index != self.scroll_tracker.selected_index() {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        if input_util.was_just_pressed(Input::Confirm) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.options
                .get(self.scroll_tracker.selected_index())
                .map(|(_, command)| *command)
        } else {
            None
        }
    }

    pub fn get_label(&self, option_index: usize) -> Option<&str> {
        self.options
            .get(option_index)
            .map(|(label, _)| label.as_str())
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if !self.open {
            return;
        }

        // resolve bounds
        const BODY_TOP_PADDING: f32 = 3.0;

        let mut text_style = TextStyle::new(game_io, FontName::Thin);
        text_style.bounds.width = self.width;
        text_style.bounds.height = text_style.line_height();

        let line_step_y = text_style.line_height() + text_style.line_spacing;
        let options_height = line_step_y * self.scroll_tracker.view_range().len() as f32;
        let height = BODY_TOP_PADDING + options_height;

        // render frame
        self.frame.set_inner_size(Vec2::new(self.width, height));
        self.frame.draw(game_io, sprite_queue);

        // render items
        let inner_top_left = self.frame.inner_top_left();
        let mut position = inner_top_left;
        position.y += BODY_TOP_PADDING;

        for i in self.scroll_tracker.view_range() {
            text_style.bounds.set_position(position);

            position.y += text_style.line_height() + text_style.line_spacing;
            text_style.draw(game_io, sprite_queue, &self.options[i].0);
        }

        // render cursor
        self.scroll_tracker.define_cursor(
            inner_top_left + Vec2::new(-8.0, BODY_TOP_PADDING + 1.0),
            line_step_y,
        );

        self.scroll_tracker.draw_cursor(sprite_queue);

        // render arrows
        self.frame
            .draw_scroll_arrows(game_io, sprite_queue, &self.scroll_tracker);
    }
}
