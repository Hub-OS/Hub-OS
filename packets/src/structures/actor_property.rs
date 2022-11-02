use super::Direction;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
pub enum ActorProperty {
    Animation(String),
    AnimationSpeed(f32),
    X(f32),
    Y(f32),
    Z(f32),
    ScaleX(f32),
    ScaleY(f32),
    Rotation(f32),
    Direction(Direction),
    SoundEffect(String),
    SoundEffectLoop(String),
}

#[derive(Debug, Clone, PartialEq, Eq, Deserialize, Serialize)]
pub enum Ease {
    Linear,
    In,
    Out,
    InOut,
    Floor,
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
pub struct KeyFrame {
    pub property_steps: Vec<(ActorProperty, Ease)>,
    pub duration: f32,
}
