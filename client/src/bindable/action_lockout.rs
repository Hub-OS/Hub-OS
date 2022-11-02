use crate::render::FrameTime;
use serde::{Deserialize, Serialize};

// complex, and not used much, let serde handle it
#[derive(Clone, Serialize, Deserialize)]
pub enum ActionLockout {
    Animation,
    Sequence,
    Async(FrameTime),
}
