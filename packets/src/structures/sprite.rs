use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, PartialEq, Eq, Debug, Clone)]
pub enum SpriteParent {
    Widget,
    Hud,
    Actor(String),
}

#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct SpriteDefinition {
    pub texture_path: String,
    pub animation_path: String,
    pub animation_state: String,
    pub animation_loops: bool,
    pub parent: SpriteParent,
    pub parent_point: Option<String>,
    pub x: f32,
    pub y: f32,
    pub layer: i32,
}
