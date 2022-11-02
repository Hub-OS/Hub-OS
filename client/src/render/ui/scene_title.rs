use super::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct SceneTitle {
    title: &'static str,
}
impl SceneTitle {
    pub fn new(title: &'static str) -> Self {
        Self { title }
    }

    pub fn draw(&self, game_io: &GameIO<Globals>, sprite_queue: &mut SpriteColorQueue) {
        let mut style = TextStyle::new_monospace(game_io, FontStyle::Wide);
        style.letter_spacing = 0.0;
        style.bounds.set_position(Vec2::new(16.0, 5.0));
        style.draw(game_io, sprite_queue, self.title);
    }
}
