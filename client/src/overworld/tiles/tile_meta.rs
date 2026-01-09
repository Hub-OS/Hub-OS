use super::{TileClass, TileShadow, Tileset};
use crate::overworld::CustomProperties;
use crate::render::{Animator, AnimatorLoopMode, Direction};
use framework::prelude::Vec2;
use std::rc::Rc;
use structures::shapes::Shape;

pub struct TileMeta {
    pub tileset: Rc<Tileset>,
    pub id: u32,
    pub gid: u32,
    pub drawing_offset: Vec2,
    pub alignment_offset: Vec2,
    pub tile_class: TileClass,
    pub minimap: bool,
    pub shadow: TileShadow,
    pub direction: Direction,
    pub custom_properties: CustomProperties,
    pub collision_shapes: Vec<Box<dyn Shape>>,
    pub animator: Animator,
}

impl TileMeta {
    pub fn new(tileset: Rc<Tileset>, id: u32) -> Self {
        let mut animator = Animator::new();
        animator.copy_from(&tileset.animator);
        animator.set_state(&id.to_string());
        animator.set_loop_mode(AnimatorLoopMode::Loop);

        TileMeta {
            id,
            gid: tileset.first_gid + id,
            tileset,
            drawing_offset: Vec2::ZERO,
            alignment_offset: Vec2::ZERO,
            tile_class: TileClass::Undefined,
            minimap: true,
            shadow: TileShadow::Always,
            direction: Direction::None,
            custom_properties: CustomProperties::new(),
            collision_shapes: Vec::new(),
            animator,
        }
    }
}
