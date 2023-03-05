use super::{TextStyle, TextboxInterface};
use crate::render::*;
use framework::prelude::GameIO;

pub type TextboxDoorstopRemover = Box<dyn FnOnce()>;

/// Keeps the textbox open
pub struct TextboxDoorstop {
    receiver: flume::Receiver<()>,
    text: String,
    complete: bool,
}

impl TextboxDoorstop {
    pub fn new() -> (Self, TextboxDoorstopRemover) {
        let (sender, receiver) = flume::unbounded();

        let doorstop_remover = Box::new(move || {
            let _ = sender.send(());
        });

        (
            Self {
                receiver,
                text: String::new(),
                complete: false,
            },
            doorstop_remover,
        )
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
        self.complete
    }

    fn update(&mut self, _game_io: &mut GameIO, _text_style: &TextStyle, _lines: usize) {
        if self.receiver.try_recv().is_ok() {
            self.complete = true;
        }
    }

    fn draw(&mut self, _game_io: &GameIO, _sprite_queue: &mut SpriteColorQueue) {
        // no ui
    }
}
