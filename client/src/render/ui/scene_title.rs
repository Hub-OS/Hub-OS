use super::*;
use crate::{render::*, resources::TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;

pub struct SceneTitle<'a> {
    title: &'a str,
}
impl<'a> SceneTitle<'a> {
    pub fn new(title: &'a str) -> Self {
        Self { title }
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let mut style = TextStyle::new_monospace(game_io, FontStyle::Wide);
        style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        style.letter_spacing = 0.0;
        style.bounds.set_position(Vec2::new(16.0, 6.0));
        style.draw(game_io, sprite_queue, self.title);
    }
}
