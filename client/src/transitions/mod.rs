// todo: move these into the engine for reuse in other projects

mod color_fade_transition;
mod fade_transition;
mod hold_color_scene;
mod pixelate_out_transition;
mod push_transition;

use color_fade_transition::*;
use fade_transition::*;
pub use hold_color_scene::*;
use push_transition::*;

use framework::prelude::{Color, Duration, GameIO};
use packets::structures::Direction;

use crate::transitions::pixelate_out_transition::PixelateTransition;

pub const DEFAULT_PUSH_DURATION: Duration = Duration::from_millis(300);
pub const DEFAULT_FADE_DURATION: Duration = Duration::from_millis(500);
pub const BATTLE_FADE_DURATION: Duration = Duration::from_millis(1200);
// true hold duration is BATTLE_HOLD_DURATION - DRAMATIC_FADE_DURATION / 2
// total transition duration is BATTLE_HOLD_DURATION + DRAMATIC_FADE_DURATION / 2
pub const BATTLE_HOLD_DURATION: Duration = Duration::from_millis(750);

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

pub fn new_battle_init(game_io: &GameIO) -> PixelateTransition {
    PixelateTransition::new(game_io, BATTLE_FADE_DURATION)
}

pub fn new_battle(game_io: &GameIO) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, BATTLE_FADE_DURATION)
}

pub fn new_battle_pop(game_io: &GameIO) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION)
}

pub fn new_connect(game_io: &GameIO) -> ColorFadeTransition {
    ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION)
}
