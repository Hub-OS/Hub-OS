use super::ActorId;
use crate::structures::TextStyleBlueprint;
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, PartialEq, Eq, Debug, Clone)]
pub enum SpriteParent {
    Widget,
    Hud,
    Actor(ActorId),
}

#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub struct SpriteAttachment {
    pub parent: SpriteParent,
    pub parent_point: Option<String>,
    pub x: f32,
    pub y: f32,
    pub layer: i32,
}

#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub enum TextAxisAlignment {
    Start,
    Middle,
    End,
}

#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
pub enum SpriteDefinition {
    Sprite {
        texture_path: String,
        animation_path: String,
        animation_state: String,
        animation_loops: bool,
    },
    Text {
        text: String,
        text_style: TextStyleBlueprint,
        h_align: TextAxisAlignment,
        v_align: TextAxisAlignment,
    },
}

impl SpriteDefinition {
    pub fn dependencies(&self) -> Vec<&str> {
        let mut dependencies = Vec::new();

        match self {
            SpriteDefinition::Sprite {
                texture_path,
                animation_path,
                ..
            } => {
                dependencies.reserve(2);
                dependencies.push(texture_path.as_str());
                dependencies.push(animation_path.as_str());
            }
            SpriteDefinition::Text { text_style, .. } => {
                dependencies.extend(text_style.dependencies());
            }
        }

        dependencies
    }
}
