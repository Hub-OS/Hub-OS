// used to create object entities, not too useful of a file after creation

use super::ObjectData;
use crate::overworld::{Map, Tile};
use crate::parse_util::parse_or_default;
use framework::graphics::Sprite;
use framework::math::Vec2;
use framework::prelude::GameIO;

pub struct TileObject {
    pub data: ObjectData,
    pub tile: Tile,
}

impl TileObject {
    pub fn new(id: u32, tile: Tile) -> Self {
        Self {
            data: ObjectData::new(id),
            tile,
        }
    }

    pub fn create_sprite(&self, game_io: &GameIO, map: &Map, layer_index: usize) -> Option<Sprite> {
        if !self.data.visible {
            return None;
        }

        let tile_meta = map.tile_meta_for_tile(self.tile.gid)?;

        let mut sprite = Sprite::new(game_io, tile_meta.tileset.texture.clone());

        tile_meta.animator.apply(&mut sprite);

        // resolve position
        let screen = map.world_3d_to_screen(self.data.position.extend(layer_index as f32));

        sprite.set_position(screen);

        // resolve scale
        let horizontal_multiplier = if self.tile.flipped_horizontal {
            -1.0
        } else {
            1.0
        };

        let vertical_multiplier = if self.tile.flipped_vertical {
            -1.0
        } else {
            1.0
        };

        let flip_multiplier = Vec2::new(horizontal_multiplier, vertical_multiplier);

        let sprite_size = sprite.size();
        let scale = self.data.size / sprite_size;
        sprite.set_scale(scale * flip_multiplier);

        // resolve rotation
        let rotation = self.data.rotation.to_radians();
        sprite.set_rotation(rotation);

        // resolve origin
        let mut origin = (-tile_meta.alignment_offset - tile_meta.drawing_offset) * flip_multiplier;

        if self.tile.flipped_horizontal {
            origin.x += sprite_size.x;
        }

        if self.tile.flipped_vertical {
            origin.y += sprite_size.y;
        }

        sprite.set_origin(origin);

        Some(sprite)
    }
}

impl From<roxmltree::Node<'_, '_>> for TileObject {
    fn from(element: roxmltree::Node) -> TileObject {
        let gid = parse_or_default::<u32>(element.attribute("gid"));

        TileObject {
            data: ObjectData::from(element),
            tile: Tile::new(gid),
        }
    }
}
