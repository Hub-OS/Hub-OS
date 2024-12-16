use crate::overworld::CustomProperties;
use crate::render::Animator;
use framework::graphics::Texture;
use framework::prelude::Vec2;
use std::sync::Arc;
use structures::shapes::Projection;

pub struct Tileset {
    pub name: String,
    pub first_gid: u32,
    pub tile_count: u32,
    pub drawing_offset: Vec2,
    pub alignment_offset: Vec2,
    pub orientation: Projection, // used for collisions
    pub custom_properties: CustomProperties,
    pub texture: Arc<Texture>,
    pub animator: Animator,
}
