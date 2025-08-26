use super::*;
use crate::render::*;
use framework::prelude::*;

#[derive(Clone)]
pub struct Text {
    pub text: String,
    pub style: TextStyle,
}

impl From<TextStyle> for Text {
    fn from(style: TextStyle) -> Self {
        Self {
            text: String::new(),
            style,
        }
    }
}

impl Text {
    pub fn new(game_io: &GameIO, font: FontName) -> Self {
        Self {
            text: String::new(),
            style: TextStyle::new(game_io, font),
        }
    }

    pub fn new_monospace(game_io: &GameIO, font: FontName) -> Self {
        let mut text = Self::new(game_io, font);
        text.style.monospace = true;

        text
    }

    pub fn with_str(mut self, text: &str) -> Self {
        self.text = String::from(text);
        self
    }

    pub fn with_string(mut self, text: String) -> Self {
        self.text = text;
        self
    }

    pub fn with_color(mut self, color: Color) -> Self {
        self.style.color = color;
        self
    }

    pub fn with_shadow_color(mut self, color: Color) -> Self {
        self.style.shadow_color = color;
        self
    }

    pub fn with_bounds(mut self, bounds: Rect) -> Self {
        self.style.bounds = bounds;
        self
    }

    pub fn line_height(&self) -> f32 {
        self.style.line_height()
    }

    pub fn measure(&self) -> TextMetrics {
        self.style.measure(&self.text)
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        self.style.draw(game_io, sprite_queue, &self.text);
    }
}

impl UiNode for Text {
    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        self.measure().size
    }

    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        let old_bounds = self.style.bounds;

        let min_size = old_bounds.size().min(bounds.size());

        self.style.bounds.set_position(bounds.position());
        self.style.bounds.set_size(min_size);

        self.draw(game_io, sprite_queue);

        self.style.bounds = old_bounds;
    }
}
