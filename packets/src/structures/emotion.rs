use num_derive::FromPrimitive;
use serde::{Deserialize, Serialize};

#[repr(u8)]
#[derive(Default, Debug, Clone, Copy, Serialize, Deserialize, FromPrimitive)]
pub enum Emotion {
    #[default]
    Normal,
    FullSynchro,
    Angry,
    Evil,
    Anxious,
    Tired,
    Exhausted,
    Pinch,
    Focus,
    Happy,
}
