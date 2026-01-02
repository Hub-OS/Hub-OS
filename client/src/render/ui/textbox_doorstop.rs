use super::{TextStyle, TextboxInterface};
use crate::render::*;
use crate::resources::{Globals, TranslationArgs};
use framework::prelude::GameIO;
use std::rc::{Rc, Weak};

pub struct TextboxDoorstopKey(Rc<()>);

/// Keeps the textbox open
pub struct TextboxDoorstop {
    weak: Weak<()>,
    text: String,
    complete: bool,
}

impl TextboxDoorstop {
    pub fn new() -> (Self, TextboxDoorstopKey) {
        let doorstop_key = TextboxDoorstopKey(Rc::new(()));

        (
            Self {
                weak: Rc::downgrade(&doorstop_key.0),
                text: String::new(),
                complete: false,
            },
            doorstop_key,
        )
    }

    pub fn with_translated(
        mut self,
        game_io: &GameIO,
        text_key: &str,
        args: TranslationArgs,
    ) -> Self {
        let globals = Globals::from_resources(game_io);
        self.text = globals.translate_with_args(text_key, args);
        self
    }

    pub fn with_str(mut self, text: &str) -> Self {
        self.text = text.to_string();
        self
    }

    pub fn with_string(mut self, text: String) -> Self {
        self.text = text;
        self
    }
}

impl TextboxInterface for TextboxDoorstop {
    fn text(&self) -> &str {
        &self.text
    }

    fn is_complete(&self) -> bool {
        self.weak.strong_count() == 0
    }

    fn update(&mut self, _game_io: &mut GameIO, _text_style: &TextStyle, _lines: usize) {}

    fn draw(&mut self, _game_io: &GameIO, _sprite_queue: &mut SpriteColorQueue) {
        // no ui
    }
}
