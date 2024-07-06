use super::{TextStyle, TextboxInterface};
use crate::render::*;
use framework::prelude::*;

pub struct TextboxMessage {
    message: String,
    callback: Option<Box<dyn FnOnce()>>,
}

impl TextboxMessage {
    pub fn new(message: String) -> Self {
        Self {
            message,
            callback: None,
        }
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
        true
    }

    fn auto_advance(&self) -> bool {
        false
    }

    fn is_complete(&self) -> bool {
        false
    }

    fn handle_completed(&mut self) {
        if let Some(callback) = self.callback.take() {
            callback();
        }
    }

    fn update(&mut self, _game_io: &mut GameIO, _text_style: &TextStyle, _lines: usize) {}

    fn draw(
        &mut self,
        _game_io: &framework::prelude::GameIO,
        _sprite_queue: &mut SpriteColorQueue,
    ) {
    }
}

pub struct TextboxMessageAuto {
    message: String,
    close_delay: FrameTime,
    callback: Option<Box<dyn FnOnce()>>,
}

impl TextboxMessageAuto {
    pub fn new(message: String) -> Self {
        Self {
            message,
            close_delay: 0,
            callback: None,
        }
    }

    pub fn with_close_delay(mut self, close_delay: FrameTime) -> Self {
        self.close_delay = close_delay;
        self
    }

    pub fn with_callback(mut self, callback: impl FnOnce() + 'static) -> Self {
        self.callback = Some(Box::new(callback));
        self
    }
}

impl TextboxInterface for TextboxMessageAuto {
    fn text(&self) -> &str {
        &self.message
    }

    fn auto_advance(&self) -> bool {
        true
    }

    fn is_complete(&self) -> bool {
        self.close_delay <= 0
    }

    fn handle_completed(&mut self) {
        if let Some(callback) = self.callback.take() {
            callback();
        }
    }

    fn update(&mut self, _game_io: &mut GameIO, _text_style: &TextStyle, _lines: usize) {
        if self.close_delay > 0 {
            self.close_delay -= 1
        }
    }

    fn draw(
        &mut self,
        _game_io: &framework::prelude::GameIO,
        _sprite_queue: &mut SpriteColorQueue,
    ) {
    }
}
