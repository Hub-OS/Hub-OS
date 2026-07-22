use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::{AssetManager, Globals};
use framework::common::GameIO;
use framework::math::Vec2;
use std::f32;
use std::sync::Arc;

#[derive(Clone)]
pub struct Emblem {
    texture_path: Arc<str>,
    spin_time: Option<FrameTime>,
}

const MAX_SPIN: FrameTime = 18;

impl Emblem {
    pub fn new(texture_path: &str) -> Self {
        Self {
            texture_path: texture_path.into(),
            spin_time: None,
        }
    }

    pub fn spin(&mut self) {
        self.spin_time = Some(0);
    }

    pub fn cancel_spin(&mut self) {
        self.spin_time = None;
    }

    pub fn update(&mut self) {
        let Some(spin_time) = &mut self.spin_time else {
            return;
        };

        *spin_time += 1;

        if *spin_time > MAX_SPIN {
            self.spin_time = None;
        }
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, position: Vec2) {
        let globals = Globals::from_resources(game_io);
        let mut sprite = globals.assets.new_sprite(game_io, &self.texture_path);
        sprite.set_position(position);
        sprite.set_origin(sprite.size() * 0.5);

        if let Some(time) = self.spin_time {
            let progress = time as f32 / MAX_SPIN as f32;

            let scale = 1.0 + (progress * f32::consts::PI).sin() * 0.1;
            sprite.set_scale(Vec2::splat(scale));

            let inverted_progress = 1.0 - progress;
            sprite.set_rotation(
                (1.0 - inverted_progress * inverted_progress * inverted_progress)
                    * f32::consts::TAU,
            );
        }

        sprite_queue.draw_sprite(&sprite);
    }
}
