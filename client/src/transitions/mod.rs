// todo: move these into the engine for reuse in other projects

mod color_fade_transition;
mod push_transition;
mod swipe_in_transition;

use color_fade_transition::*;
use push_transition::*;
use swipe_in_transition::*;

use crate::resources::Globals;
use framework::prelude::{Color, Duration, GameIO};
use packets::structures::Direction;

pub const DEFAULT_PUSH_DURATION: Duration = Duration::from_millis(300);
pub const DEFAULT_FADE_DURATION: Duration = Duration::from_millis(500);
pub const DRAMATIC_FADE_DURATION: Duration = Duration::from_millis(1000);

pub fn new_boot(game_io: &GameIO<Globals>) -> SwipeInTransition {
    SwipeInTransition::new(
        game_io,
        game_io.globals().default_sampler.clone(),
        Direction::Down,
        Duration::from_secs_f32(0.5),
    )
}

pub fn new_navigation(game_io: &GameIO<Globals>) -> PushTransition {
    PushTransition::new(
        game_io,
        game_io.globals().default_sampler.clone(),
        Direction::Right,
        DEFAULT_PUSH_DURATION,
    )
}

pub fn new_scene_pop(game_io: &GameIO<Globals>) -> PushTransition {
    PushTransition::new(
        game_io,
        game_io.globals().default_sampler.clone(),
        Direction::Left,
        DEFAULT_PUSH_DURATION,
    )
}

pub fn new_sub_scene(game_io: &GameIO<Globals>) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::BLACK, DEFAULT_FADE_DURATION)
}

pub fn new_sub_scene_pop(game_io: &GameIO<Globals>) -> ColorFadeTransition {
    new_sub_scene(game_io)
}

pub fn new_battle(game_io: &GameIO<Globals>) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, DRAMATIC_FADE_DURATION)
}

pub fn new_battle_pop(game_io: &GameIO<Globals>) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION)
}

pub fn new_connect(game_io: &GameIO<Globals>) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION)
}
