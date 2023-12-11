use super::Map;
use crate::render::SpriteColorQueue;
use framework::prelude::*;
use std::cmp::Ordering;

struct SortableSpriteReference {
    sprite: Sprite,
    position: Vec3,
    screen_y: f32,
    z: i32,
}

#[derive(Default)]
pub struct OverworldSpriteLayer {
    sprites: Vec<SortableSpriteReference>,
}

impl OverworldSpriteLayer {
    pub fn add_sprite(&mut self, map: &Map, sprite: Sprite, position: Vec3) {
        self.sprites.push(SortableSpriteReference {
            sprite,
            position,
            screen_y: map.world_to_screen(position.xy()).y,
            z: position.z.ceil() as i32,
        });
    }

    pub fn sort(&mut self) {
        self.sprites.sort_unstable_by(|a, b| {
            (a.z, a.screen_y)
                .partial_cmp(&(b.z, b.screen_y))
                .unwrap_or(Ordering::Equal)
        });
    }

    pub fn draw(&mut self, map: &Map, sprite_queue: &mut SpriteColorQueue) {
        for sprite_ref in &mut self.sprites {
            let sprite = &mut sprite_ref.sprite;

            // shade
            let original_color = sprite.color();

            if map.is_in_shadow(sprite_ref.position) {
                sprite.set_color(original_color.multiply_color(Map::SHADOW_MULTIPLIER));
            }

            // draw
            sprite_queue.draw_sprite(sprite);
        }
    }
}
