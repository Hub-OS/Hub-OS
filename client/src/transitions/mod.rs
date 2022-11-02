// todo: move these into the engine for reuse in other projects

mod color_fade_transition;
mod push_transition;
mod swipe_in_transition;

pub use color_fade_transition::*;
pub use push_transition::*;
pub use swipe_in_transition::*;

use framework::prelude::Duration;
pub const DEFAULT_PUSH_DURATION: Duration = Duration::from_millis(300);
pub const DEFAULT_FADE_DURATION: Duration = Duration::from_millis(500);
pub const DRAMATIC_FADE_DURATION: Duration = Duration::from_millis(1000);
