use crate::render::{Animator, Background};
use crate::resources::*;
use framework::prelude::*;

#[derive(Default, PartialEq)]
pub struct BackgroundProperties {
    pub texture_path: String,
    pub animation_path: String,
    pub velocity: Option<Vec2>,
    pub parallax: f32,
}

impl BackgroundProperties {
    pub fn generate_background(&self, game_io: &GameIO, assets: &impl AssetManager) -> Background {
        let sprite = assets.new_sprite(game_io, &self.texture_path);
        let animator = Animator::from(&assets.text(&self.animation_path));

        let mut background = Background::new(animator, sprite);

        if let Some(velocity) = self.velocity {
            background.set_velocity(velocity);
        }

        background
    }
}
