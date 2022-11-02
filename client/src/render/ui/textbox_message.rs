use super::{TextStyle, TextboxInterface};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct TextboxMessage {
    message: String,
    auto_advance: bool,
    completable: bool,
    callback: Option<Box<dyn FnOnce()>>,
}

impl TextboxMessage {
    pub fn new(message: String) -> Self {
        Self {
            message,
            auto_advance: false,
            completable: true,
            callback: None,
        }
    }

    pub fn new_auto(message: String) -> Self {
        Self {
            message,
            auto_advance: true,
            completable: true,
            callback: None,
        }
    }

    pub fn with_completable(mut self, completable: bool) -> Self {
        self.completable = completable;
        self
    }

    pub fn with_callback(mut self, callback: impl FnOnce() + 'static) -> Self {
        self.callback = Some(Box::new(callback));
        self
    }
}

impl TextboxInterface for TextboxMessage {
    fn text(&self) -> &str {
        &self.message
    }

    fn completes_with_indicator(&self) -> bool {
        self.completable
    }

    fn auto_advance(&self) -> bool {
        self.auto_advance
    }

    fn is_complete(&self) -> bool {
        false
    }

    fn handle_completed(&mut self) {
        if let Some(callback) = self.callback.take() {
            callback();
        }
    }

    fn update(&mut self, _game_io: &mut GameIO<Globals>, _text_style: &TextStyle, _lines: usize) {}

    fn draw(
        &mut self,
        _game_io: &framework::prelude::GameIO<Globals>,
        _sprite_queue: &mut SpriteColorQueue,
    ) {
    }
}
