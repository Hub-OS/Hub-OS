use crate::render::ui::{FontStyle, TextStyle};
use crate::render::*;
use crate::resources::TEXT_DARK_SHADOW_COLOR;
use framework::prelude::*;

pub struct OptionTip {
    action_name: String,
    top_right: Vec2,
}

impl OptionTip {
    pub fn new(action_name: String, top_right: Vec2) -> Self {
        Self {
            action_name,
            top_right,
        }
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        const LABEL: &str = "OPTION:";

        let mut text_style = TextStyle::new(game_io, FontStyle::Context);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(self.top_right);

        // draw action name
        text_style.color = Color::GREEN;
        text_style.bounds.x -= text_style.measure(&self.action_name).size.x;
        text_style.draw(game_io, sprite_queue, &self.action_name);

        // draw label
        text_style.color = Color::WHITE;
        text_style.bounds.x -= text_style.measure(LABEL).size.x + text_style.letter_spacing;
        text_style.draw(game_io, sprite_queue, LABEL);
    }
}
