use super::{Tile, VOID_TILE};
use framework::prelude::*;

pub struct TileLayer {
    visible: bool,
    cols: u32,
    rows: u32,
    tiles: Vec<Tile>,
    tiles_modified: bool,
}

impl TileLayer {
    pub fn new(cols: u32, rows: u32) -> Self {
        Self {
            visible: true,
            cols,
            rows,
            tiles: vec![VOID_TILE; (cols * rows) as usize],
            tiles_modified: false,
        }
    }

    pub fn tile_at(&self, point: IVec2) -> &Tile {
        let (x, y) = point.into();

        if x < 0 || y < 0 || x >= self.cols as i32 || y >= self.rows as i32 {
            return &VOID_TILE;
        }

        &self.tiles[(y * self.cols as i32 + x) as usize]
    }

    pub fn tile_at_f32(&self, point: Vec2) -> &Tile {
        self.tile_at(point.floor().as_ivec2())
    }

    pub fn set_tile(&mut self, point: IVec2, tile: Tile) {
        let (x, y) = point.into();

        if x < 0 || y < 0 || x >= self.cols as i32 || y >= self.rows as i32 {
            return;
        }

        self.tiles[(y * self.cols as i32 + x) as usize] = tile;
        self.tiles_modified = true;
    }

    pub fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }

    pub fn visible(&self) -> bool {
        self.visible
    }
}
