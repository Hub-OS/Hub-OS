// todo: move these into the engine for reuse in other projects

mod color_fade_transition;
mod fade_transition;
mod push_transition;

use color_fade_transition::*;
use fade_transition::*;
use push_transition::*;

use framework::prelude::{Color, Duration, GameIO};
use packets::structures::Direction;

pub const DEFAULT_PUSH_DURATION: Duration = Duration::from_millis(300);
pub const DEFAULT_FADE_DURATION: Duration = Duration::from_millis(500);
pub const DRAMATIC_FADE_DURATION: Duration = Duration::from_millis(1000);

pub fn new_boot(game_io: &GameIO) -> FadeTransition {
    FadeTransition::new(game_io, Duration::from_secs_f32(0.5))
}

pub fn new_navigation(game_io: &GameIO) -> PushTransition {
    PushTransition::new(game_io, Direction::Right, DEFAULT_PUSH_DURATION)
}

pub fn new_scene_pop(game_io: &GameIO) -> PushTransition {
    PushTransition::new(game_io, Direction::Left, DEFAULT_PUSH_DURATION)
}

pub fn new_sub_scene(game_io: &GameIO) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::BLACK, DEFAULT_FADE_DURATION)
}

pub fn new_sub_scene_pop(game_io: &GameIO) -> ColorFadeTransition {
    new_sub_scene(game_io)
}

pub fn new_battle(game_io: &GameIO) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, DRAMATIC_FADE_DURATION)
}

pub fn new_battle_pop(game_io: &GameIO) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION)
}

pub fn new_connect(game_io: &GameIO) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION)
}
