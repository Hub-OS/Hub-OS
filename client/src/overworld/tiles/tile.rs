use crate::overworld::Map;
use crate::render::Direction;
use framework::prelude::*;
use structures::shapes::Projection;

use super::*;

pub const VOID_TILE: Tile = Tile::new(0);

#[derive(Clone, Copy)]
pub struct Tile {
    pub gid: u32,
    pub flipped_horizontal: bool,
    pub flipped_vertical: bool,
    pub rotated: bool,
}

impl Tile {
    pub const fn new(gid: u32) -> Tile {
        // https://doc.mapeditor.org/en/stable/reference/tmx-world-format/#tile-flipping
        let flipped_horizontal = (gid >> 31 & 1) == 1;
        let flipped_vertical = (gid >> 30 & 1) == 1;
        let flipped_anti_diagonal = (gid >> 29 & 1) == 1;

        let gid = gid << 3 >> 3;
        let rotated = flipped_anti_diagonal;

        let mut tile = Tile {
            gid,
            flipped_horizontal,
            flipped_vertical,
            rotated,
        };

        if rotated {
            tile.flipped_horizontal = flipped_vertical;
            tile.flipped_vertical = !flipped_horizontal;
        }

        tile
    }

    pub fn get_direction(&self, meta: &TileMeta) -> Direction {
        let mut direction = meta.direction;

        if self.flipped_horizontal {
            direction = direction.horizontal_mirror();
        }

        if self.flipped_vertical {
            direction = direction.vertical_mirror();
        }

        direction
    }

    pub fn intersects(&self, map: &Map, point: Vec2) -> bool {
        let tile_meta = match map.tile_meta_for_tile(self.gid) {
            Some(tileset) => tileset,
            _ => return true,
        };

        let mut test_position = point;

        // convert to orthogonal to simplify transformations
        test_position = map.world_to_screen(test_position);

        let sprite_size = tile_meta
            .animator
            .current_frame()
            .map(|frame| frame.size())
            .unwrap_or_default();

        let tile_size = map.tile_size();

        if self.rotated {
            let tile_center = Vec2::new(0.0, (tile_size.y / 2) as f32);

            test_position -= tile_center;
            // rotate position counter clockwise
            test_position = Vec2::new(test_position.y, -test_position.x);
            test_position += tile_center;
        }

        test_position -= tile_meta.drawing_offset;

        if self.flipped_horizontal {
            test_position.x *= -1.0;
        }

        if self.flipped_vertical {
            test_position.y = sprite_size.y - test_position.y;
        }

        if matches!(tile_meta.tileset.orientation, Projection::Orthographic) {
            // tiled uses position on sprite with orthographic projection

            test_position.x += (tile_size.x / 2) as f32;
            test_position.y += sprite_size.y - tile_size.y as f32;
        } else {
            // isometric orientation
            test_position = map.screen_to_world(test_position);
        }

        for shape in &tile_meta.collision_shapes {
            if shape.intersects(test_position.into()) {
                return true;
            }
        }

        false
    }
}
