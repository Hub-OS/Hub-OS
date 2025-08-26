use super::Map;
use crate::render::{ui::Text, SpriteColorQueue};
use framework::prelude::*;
use std::cmp::Ordering;

struct SortableSpriteReference {
    world_index: usize,
    entity: hecs::Entity,
    position: Vec3,
    screen_y: f32,
}

#[derive(Default)]
pub struct OverworldSpriteLayer {
    sprites: Vec<SortableSpriteReference>,
}

impl OverworldSpriteLayer {
    pub fn add_sprite(
        &mut self,
        map: &Map,
        world_index: usize,
        entity: hecs::Entity,
        position: Vec3,
    ) {
        self.sprites.push(SortableSpriteReference {
            world_index,
            entity,
            position,
            screen_y: map.world_to_screen(position.xy()).y,
        });
    }

    pub fn sort(&mut self) {
        self.sprites.sort_unstable_by(|a, b| {
            a.screen_y
                .partial_cmp(&b.screen_y)
                .unwrap_or(Ordering::Equal)
        });
    }

    pub fn draw(
        &mut self,
        game_io: &GameIO,
        map: &Map,
        worlds: &[&hecs::World],
        sprite_queue: &mut SpriteColorQueue,
    ) {
        type Query<'a> = (Option<&'a mut Sprite>, Option<&'a mut Text>);

        for sprite_ref in &mut self.sprites {
            let world = &worlds[sprite_ref.world_index];
            let Ok(mut query) = world.query_one::<Query>(sprite_ref.entity) else {
                continue;
            };

            let Some((sprite, text)) = query.get() else {
                continue;
            };

            if let Some(sprite) = sprite {
                // shade
                let original_color = sprite.color();

                if map.is_in_shadow(sprite_ref.position) {
                    sprite.set_color(original_color.multiply_color(Map::SHADOW_MULTIPLIER));
                }

                // draw
                sprite_queue.draw_sprite(sprite);

                // reset color
                sprite.set_color(original_color);
            }

            if let Some(text) = text {
                // shade
                let original_color = text.style.color;
                let original_shadow_color = text.style.shadow_color;

                if map.is_in_shadow(sprite_ref.position) {
                    text.style.color = original_color.multiply_color(Map::SHADOW_MULTIPLIER);
                    text.style.shadow_color = original_color.multiply_color(Map::SHADOW_MULTIPLIER);
                }

                // draw
                text.draw(game_io, sprite_queue);

                // reset colors
                text.style.color = original_color;
                text.style.shadow_color = original_shadow_color;
            }
        }
    }
}
