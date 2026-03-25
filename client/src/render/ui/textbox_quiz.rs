use super::{TextStyle, TextboxCursor, TextboxInterface, UiInputTracker};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct TextboxQuiz {
    header: String,
    message: String,
    valid_options: [bool; 3],
    selection: usize,
    complete: bool,
    never_complete: bool,
    callback: Box<dyn Fn(usize)>,
    cursor: Option<TextboxCursor>,
    cursor_shift: usize,
    animate_cursor: bool,
    input_tracker: UiInputTracker,
}

impl TextboxQuiz {
    pub fn new(options: &[&str; 3], callback: impl Fn(usize) + 'static) -> Self {
        Self {
            header: Default::default(),
            message: format!("\x02  {}\n  {}\n  {}", options[0], options[1], options[2]),
            valid_options: options.map(|s| !s.is_empty()),
            selection: 0,
            complete: false,
            never_complete: false,
            callback: Box::new(callback),
            cursor: None,
            cursor_shift: 0,
            animate_cursor: false,
            input_tracker: UiInputTracker::new(),
        }
    }

    pub fn with_default_response(mut self, selection: usize) -> Self {
        self.selection = selection.min(2);
        self
    }

    pub fn with_never_complete(mut self, never_complete: bool) -> Self {
        self.never_complete = never_complete;
        self
    }

    pub fn with_animate_cursor(mut self, animate_cursor: bool) -> Self {
        self.animate_cursor = animate_cursor;
        self
    }

    pub fn with_header(mut self, header: String) -> Self {
        self.header = header;
        self
    }

    fn update_cursor(&mut self, text_style: &TextStyle) {
        let cursor = self.cursor.as_mut().unwrap();

        let cursor_line = self.cursor_shift + self.selection;
        let line_height = text_style.line_height();
        let relative_position = Vec2::new(
            text_style.measure("  ").size.x,
            line_height * 0.5 + line_height * cursor_line as f32,
        );

        cursor.set_position(text_style.bounds.position() + relative_position);
        cursor.update();
    }

    fn handle_input(&mut self, game_io: &GameIO, text_style: &TextStyle) {
        let input_util = InputUtil::new(game_io);
        self.input_tracker.update(game_io);

        let old_selection = self.selection;

        for _ in 0..2 {
            if self.input_tracker.pulsed(Input::Up) {
                if self.selection == 0 {
                    self.selection = 2;
                } else {
                    self.selection -= 1;
                }
            }

            if self.input_tracker.pulsed(Input::Down) {
                if self.selection == 2 {
                    self.selection = 0;
                } else {
                    self.selection += 1;
                }
            }

            if self.valid_options.get(self.selection) == Some(&true) {
                break;
            }
        }

        if self.selection != old_selection {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);
            self.update_cursor(text_style);
        }

        if input_util.was_just_pressed(Input::Confirm) {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.complete = !self.never_complete;

            (self.callback)(self.selection);
        }
    }
}

impl TextboxInterface for TextboxQuiz {
    fn prepare_text(&mut self, text_style: &TextStyle) {
        if self.header.is_empty() {
            return;
        }

        let header = std::mem::take(&mut self.header);

        let metrics = text_style.measure(&header);
        let header_lines = metrics.line_ranges.len();
        let option_lines =
            self.valid_options.len() - self.valid_options.iter().rev().take_while(|&&v| !v).count();

        if header_lines + option_lines <= 3 {
            self.message = format!("{header}\n{}", self.message.trim_end());
            self.cursor_shift = metrics.line_ranges.len();
        }
    }

    fn text(&self) -> &str {
        &self.message
    }

    fn is_complete(&self) -> bool {
        self.complete
    }

    fn update(&mut self, game_io: &mut GameIO, text_style: &TextStyle, _lines: usize) {
        if self.complete {
            return;
        }

        if self.cursor.is_none() {
            self.cursor = Some(TextboxCursor::new(game_io));
            self.update_cursor(text_style);
        }

        if self.animate_cursor {
            self.update_cursor(text_style);
        }

        if !game_io.is_in_transition() {
            self.handle_input(game_io, text_style);
        }
    }

    fn draw(&mut self, game_io: &framework::prelude::GameIO, sprite_queue: &mut SpriteColorQueue) {
        if game_io.is_in_transition() {
            return;
        }

        let Some(cursor) = &mut self.cursor else {
            return;
        };

        cursor.draw(sprite_queue);
    }
}
