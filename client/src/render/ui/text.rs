use super::*;
use crate::render::*;
use framework::prelude::*;

#[derive(Clone)]
pub struct Text {
    pub text: String,
    pub style: TextStyle,
}

impl Text {
    pub fn new(game_io: &GameIO, font_style: FontStyle) -> Self {
        Self {
            text: String::new(),
            style: TextStyle::new(game_io, font_style),
        }
    }

    pub fn new_monospace(game_io: &GameIO, font_style: FontStyle) -> Self {
        let mut text = Self::new(game_io, font_style);
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

    pub fn with_text_style(mut self, style: TextStyle) -> Self {
        self.style = style;
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
