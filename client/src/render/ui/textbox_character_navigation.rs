use super::{TextStyle, TextboxCursor, TextboxInterface, UiInputTracker};
use crate::render::*;
use crate::resources::*;
use framework::prelude::GameIO;
use framework::prelude::*;

pub struct TextboxCharacterNavigation {
    selection: usize,
    callback: Box<dyn Fn(usize)>,
    cursor: Option<TextboxCursor>,
    input_tracker: UiInputTracker,
}

impl TextboxCharacterNavigation {
    pub fn new(callback: impl Fn(usize) + 'static) -> Self {
        Self {
            selection: 0,
            callback: Box::new(callback),
            cursor: None,
            input_tracker: UiInputTracker::new(),
        }
    }

    fn handle_input(&mut self, game_io: &GameIO) {
        let input_util = InputUtil::new(game_io);
        self.input_tracker.update(game_io);

        let old_selection = self.selection;

        if self.input_tracker.is_active(Input::Up) {
            self.selection += 1;
        }

        if self.input_tracker.is_active(Input::Down) {
            self.selection += 1;
        }

        self.selection %= 2;

        if self.selection != old_selection {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        if input_util.was_just_pressed(Input::Confirm) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);

            (self.callback)(self.selection);
        }
    }
}

impl TextboxInterface for TextboxCharacterNavigation {
    fn text(&self) -> &str {
        "Here is my status.\x02\n  Customize\n  Switch"
    }

    fn is_complete(&self) -> bool {
        false
    }

    fn update(&mut self, game_io: &mut GameIO, text_style: &TextStyle, _lines: usize) {
        if self.cursor.is_none() {
            self.cursor = Some(TextboxCursor::new(game_io));
        }

        if !game_io.is_in_transition() {
            self.handle_input(game_io);
        }

        let cursor = self.cursor.as_mut().unwrap();

        let cursor_line = 1.0 + self.selection as f32;

        let line_height = text_style.line_height();
        let relative_position = Vec2::new(
            text_style.measure("  ").size.x,
            line_height * 0.5 + line_height * cursor_line,
        );

        cursor.set_position(text_style.bounds.position() + relative_position);
        cursor.update();
    }

    fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if game_io.is_in_transition() {
            return;
        }

        let cursor = match &mut self.cursor {
            Some(cursor) => cursor,
            None => return,
        };

        cursor.draw(sprite_queue);
    }
}
