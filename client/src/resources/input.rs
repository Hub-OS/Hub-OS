use super::Globals;
use crate::saves::Config;
use framework::prelude::*;
use packets::structures::Direction;

pub use crate::bindable::Input;

pub struct InputUtil<'a> {
    input_manager: &'a InputManager,
    config: &'a Config,
}

impl<'a> InputUtil<'a> {
    pub fn new(game_io: &'a GameIO<Globals>) -> Self {
        Self {
            input_manager: game_io.input(),
            config: &game_io.globals().config,
        }
    }

    pub fn latest_input(&self) -> Option<Input> {
        self.input_manager.latest_key().and_then(|key| {
            self.config
                .key_bindings
                .iter()
                .find(|(_, k)| **k == key)
                .map(|(input, _)| *input)
        })
    }

    pub fn direction(&self) -> Direction {
        let x = self.as_axis(Input::Left, Input::Right);
        let y = self.as_axis(Input::Up, Input::Down);

        let horizontal = if x < 0.0 {
            Direction::Left
        } else if x > 0.0 {
            Direction::Right
        } else {
            Direction::None
        };

        let vertical = if y < 0.0 {
            Direction::Up
        } else if y > 0.0 {
            Direction::Down
        } else {
            Direction::None
        };

        horizontal.join(vertical)
    }

    pub fn is_down(&self, input: Input) -> bool {
        match self.config.key_bindings.get(&input) {
            Some(key) => self.input_manager.is_key_down(*key),
            None => false,
        }
    }

    pub fn was_just_pressed(&self, input: Input) -> bool {
        match self.config.key_bindings.get(&input) {
            Some(key) => self.input_manager.was_key_just_pressed(*key),
            None => false,
        }
    }

    pub fn was_released(&self, input: Input) -> bool {
        match self.config.key_bindings.get(&input) {
            Some(key) => self.input_manager.was_key_released(*key),
            None => false,
        }
    }

    pub fn as_axis(&self, negative: Input, positive: Input) -> f32 {
        let mut value = 0.0;

        if self.is_down(negative) {
            value -= 1.0;
        }

        if self.is_down(positive) {
            value += 1.0;
        }

        value
    }
}
