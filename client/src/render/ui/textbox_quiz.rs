use super::{TextStyle, TextboxCursor, TextboxInterface, UiInputTracker};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct TextboxQuiz {
    message: String,
    selection: usize,
    complete: bool,
    callback: Option<Box<dyn FnOnce(usize)>>,
    cursor: Option<TextboxCursor>,
    input_tracker: UiInputTracker,
}

impl TextboxQuiz {
    pub fn new(options: &[&str; 3], callback: impl FnOnce(usize) + 'static) -> Self {
        Self {
            message: format!("\x02  {}\n  {}\n  {}", options[0], options[1], options[2]),
            complete: false,
            selection: 0,
            callback: Some(Box::new(callback)),
            cursor: None,
            input_tracker: UiInputTracker::new(),
        }
    }

    fn update_cursor(&mut self, text_style: &TextStyle) {
        let cursor = self.cursor.as_mut().unwrap();

        let line_height = text_style.line_height();
        let relative_position = Vec2::new(
            text_style.measure("  ").size.x,
            line_height * 0.5 + line_height * self.selection as f32,
        );

        cursor.set_position(text_style.bounds.position() + relative_position);
        cursor.update();
    }

    fn handle_input(&mut self, game_io: &GameIO, text_style: &TextStyle) {
        let input_util = InputUtil::new(game_io);
        self.input_tracker.update(game_io);

        let old_selection = self.selection;

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

        if self.selection != old_selection {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);
            self.update_cursor(text_style);
        }

        if input_util.was_just_pressed(Input::Confirm) {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.complete = true;
        }

        if self.complete {
            let callback = self.callback.take().unwrap();
            callback(self.selection);
        }
    }
}

impl TextboxInterface for TextboxQuiz {
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

        if !game_io.is_in_transition() {
            self.handle_input(game_io, text_style);
        }
    }

    fn draw(&mut self, _game_io: &framework::prelude::GameIO, sprite_queue: &mut SpriteColorQueue) {
        let Some(cursor) = &mut self.cursor else {
            return;
        };

        cursor.draw(sprite_queue);
    }
}
