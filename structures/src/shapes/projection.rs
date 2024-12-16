#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Projection {
    Isometric,
    Orthographic,
}

impl Projection {
    pub fn screen_to_world(self, tile_size: (f32, f32), point: (f32, f32)) -> (f32, f32) {
        match self {
            Projection::Isometric => {
                let vertical_scale = tile_size.1 / tile_size.0;

                (
                    point.1 + point.0 * vertical_scale,
                    point.1 - point.0 * vertical_scale,
                )
            }
            Projection::Orthographic => point,
        }
    }

    pub fn world_to_screen(self, tile_size: (f32, f32), point: (f32, f32)) -> (f32, f32) {
        match self {
            Projection::Isometric => {
                let vertical_scale = tile_size.1 / tile_size.0;

                (point.0 - point.1, (point.0 + point.1) * vertical_scale)
            }
            Projection::Orthographic => point,
        }
    }

    pub fn world_3d_to_screen(self, tile_size: (f32, f32), point: (f32, f32, f32)) -> (f32, f32) {
        match self {
            Projection::Isometric => {
                let vertical_scale = tile_size.1 / tile_size.0;

                (
                    point.0 - point.1,
                    (point.0 + point.1) * vertical_scale - point.2 * tile_size.1 * 0.5,
                )
            }
            Projection::Orthographic => (point.0, point.1 - point.2 * tile_size.1 * 0.5),
        }
    }
}
