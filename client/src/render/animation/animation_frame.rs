use super::FrameTime;
use framework::prelude::*;
use indexmap::IndexMap;
use uncased::{Uncased, UncasedStr};

#[derive(Default, Clone, Debug)]
pub struct AnimationFrame {
    pub duration: FrameTime,
    pub bounds: Rect,
    pub origin: Vec2,
    pub points: IndexMap<Uncased<'static>, Vec2>,
    pub valid: bool,
}

impl AnimationFrame {
    pub fn size(&self) -> Vec2 {
        self.bounds.size().abs()
    }

    pub fn point(&self, name: &str) -> Option<Vec2> {
        self.points.get(<&UncasedStr>::from(name)).cloned()
    }

    pub fn apply(&self, sprite: &mut Sprite) {
        if self.valid {
            sprite.set_origin(self.origin);
            sprite.set_frame(self.bounds);
        }
    }
}
