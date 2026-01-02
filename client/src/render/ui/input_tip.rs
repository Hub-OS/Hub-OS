use crate::render::ui::{FontName, TextStyle};
use crate::render::*;
use crate::resources::{Globals, TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;
use packets::structures::Input;

pub struct InputTip {
    label: String,
    action_name: String,
    top_right: Vec2,
}

impl InputTip {
    pub fn new(game_io: &GameIO, input: Input, action_key: &str, top_right: Vec2) -> Self {
        let globals = Globals::from_resources(game_io);

        Self {
            label: globals.translate(input.translation_key()).to_uppercase() + ":",
            action_name: globals.translate(action_key),
            top_right,
        }
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let mut text_style = TextStyle::new(game_io, FontName::Context);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(self.top_right);

        // draw action name
        text_style.color = Color::GREEN;
        text_style.bounds.x -= text_style.measure(&self.action_name).size.x;
        text_style.draw(game_io, sprite_queue, &self.action_name);

        // draw label
        text_style.color = Color::WHITE;
        text_style.bounds.x -= text_style.measure(&self.label).size.x + text_style.letter_spacing;
        text_style.draw(game_io, sprite_queue, &self.label);
    }
}
