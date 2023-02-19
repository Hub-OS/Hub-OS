use super::Direction;
use enum_map::Enum;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
pub struct ActorKeyFrame {
    pub property_steps: Vec<(ActorProperty, Ease)>,
    pub duration: f32,
}

// based on https://developer.mozilla.org/en-US/docs/Web/CSS/easing-function
#[derive(Debug, Clone, Copy, PartialEq, Eq, Deserialize, Serialize)]
pub enum Ease {
    Linear,
    In,
    Out,
    InOut,
    Floor, // similar to steps
}

impl Ease {
    pub fn interpolate(self, a: f32, b: f32, mut progress: f32) -> f32 {
        progress = progress.clamp(0.0, 1.0);

        // curves based on https://developer.mozilla.org/en-US/docs/Web/CSS/easing-function
        // not using bezier but hoping it's close enough
        let curve = match self {
            Self::Linear => progress,
            Self::In => progress.powf(1.7),
            Self::Out => 2.0 * progress - progress.powf(1.7),
            Self::InOut => (-2.0 * progress * progress * progress) + (3.0 * progress * progress),
            Self::Floor => progress.floor(),
        };

        (b - a) * curve + a
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Enum)]
pub enum ActorPropertyId {
    Animation,
    AnimationSpeed,
    X,
    Y,
    Z,
    ScaleX,
    ScaleY,
    Rotation,
    Direction,
    SoundEffect,
    SoundEffectLoop,
}

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

impl ActorProperty {
    pub fn get_f32(&self) -> f32 {
        match self {
            Self::AnimationSpeed(v)
            | Self::X(v)
            | Self::Y(v)
            | Self::Z(v)
            | Self::ScaleX(v)
            | Self::ScaleY(v)
            | Self::Rotation(v) => *v,
            _ => 0.0,
        }
    }

    pub fn get_str(&self) -> &str {
        match self {
            Self::Animation(v) | Self::SoundEffect(v) | Self::SoundEffectLoop(v) => v,
            _ => "",
        }
    }

    pub fn id(&self) -> ActorPropertyId {
        match self {
            Self::Animation(_) => ActorPropertyId::Animation,
            Self::AnimationSpeed(_) => ActorPropertyId::AnimationSpeed,
            Self::X(_) => ActorPropertyId::X,
            Self::Y(_) => ActorPropertyId::Y,
            Self::Z(_) => ActorPropertyId::Z,
            Self::ScaleX(_) => ActorPropertyId::ScaleX,
            Self::ScaleY(_) => ActorPropertyId::ScaleY,
            Self::Rotation(_) => ActorPropertyId::Rotation,
            Self::Direction(_) => ActorPropertyId::Direction,
            Self::SoundEffect(_) => ActorPropertyId::SoundEffect,
            Self::SoundEffectLoop(_) => ActorPropertyId::SoundEffectLoop,
        }
    }
}
