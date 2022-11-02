use super::{Map, TileClass, TileShadow};

pub struct ShadowMap {
    cols: usize,
    rows: usize,
    shadows: Vec<usize>,
}

impl ShadowMap {
    pub fn new(cols: usize, rows: usize) -> Self {
        ShadowMap {
            cols,
            rows,
            shadows: vec![0; cols * rows],
        }
    }

    pub fn calculate_shadows(&mut self, map: &Map) {
        let map_layers = map.tile_layers();

        for y in 0..self.rows {
            for x in 0..self.cols {
                let shadow_index = calculate_index(self.cols, x, y);
                self.shadows[shadow_index] = 0;

                for index in (1..map_layers.len()).rev() {
                    let layer = &map_layers[index];

                    let tile = layer.tile_at((x as i32, y as i32).into());

                    let tile_meta = match map.tile_meta_for_tile(tile.gid) {
                        Some(tile_meta) => tile_meta,
                        _ => continue, // void
                    };

                    match tile_meta.shadow {
                        TileShadow::Automatic => {
                            // invisible tile
                            if tile_meta.tile_class == TileClass::Invisible {
                                continue;
                            }

                            // hole
                            if map
                                .ignore_tile_above((x as i32, y as i32).into(), (index - 1) as i32)
                            {
                                continue;
                            }
                        }
                        TileShadow::Always => {}
                        TileShadow::Never => {
                            continue;
                        }
                    }

                    self.shadows[shadow_index] = index;
                    break;
                }
            }
        }
    }

    pub fn has_shadow(&self, position: (i32, i32), layer: usize) -> bool {
        let (x, y) = position;

        let cols = self.cols as i32;
        let rows = self.rows as i32;

        if !(0..cols).contains(&x) || !(0..rows).contains(&y) {
            return false;
        }

        let shadow_index = calculate_index(self.cols, x as usize, y as usize);

        if shadow_index > self.shadows.len() {
            return false;
        }

        layer < self.shadows[shadow_index]
    }
}

fn calculate_index(cols: usize, x: usize, y: usize) -> usize {
    y * cols + x
}
