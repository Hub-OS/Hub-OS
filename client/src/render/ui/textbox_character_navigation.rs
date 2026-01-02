use super::{TextStyle, TextboxCursor, TextboxInterface, UiInputTracker};
use crate::render::*;
use crate::resources::*;
use framework::prelude::GameIO;
use framework::prelude::*;

const TOTAL_OPTIONS: usize = 3;

pub struct TextboxCharacterNavigation {
    text: String,
    selection: usize,
    callback: Box<dyn Fn(usize)>,
    cursor: Option<TextboxCursor>,
    input_tracker: UiInputTracker,
}

impl TextboxCharacterNavigation {
    pub fn new(game_io: &GameIO, callback: impl Fn(usize) + 'static) -> Self {
        let globals = Globals::from_resources(game_io);

        Self {
            text: format!(
                "\x02  {}\n  {}\n  {}",
                globals.translate("character-status-blocks-option"),
                globals.translate("character-status-switch-drives-option"),
                globals.translate("character-status-navis-option")
            ),
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

        if self.selection == 0 {
            self.selection = TOTAL_OPTIONS;
        }

        if self.input_tracker.pulsed(Input::Up) {
            self.selection -= 1;
        }

        if self.input_tracker.pulsed(Input::Down) {
            self.selection += 1;
        }

        self.selection %= TOTAL_OPTIONS;

        if self.selection != old_selection {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        if input_util.was_just_pressed(Input::Confirm) {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);

            (self.callback)(self.selection);
        }
    }
}

impl TextboxInterface for TextboxCharacterNavigation {
    fn text(&self) -> &str {
        &self.text
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

        let cursor_line = self.selection as f32;

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

        let Some(cursor) = &mut self.cursor else {
            return;
        };

        cursor.draw(sprite_queue);
    }
}
