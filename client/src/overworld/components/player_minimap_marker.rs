use framework::prelude::*;

const CONCEALED_COLOR_MULTIPLIER: f32 = 0.65; // 65% transparency, similar to world sprite darkening

#[derive(Default)]
pub struct PlayerMinimapMarker {
    pub color: Color,
}

impl PlayerMinimapMarker {
    pub fn new_player() -> Self {
        Self {
            color: Color::from((0, 248, 248, 255)),
        }
    }
}
