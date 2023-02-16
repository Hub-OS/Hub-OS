use super::{Globals, Input};
use crate::saves::Config;
use framework::prelude::*;
use packets::structures::Direction;
use std::collections::HashMap;

pub struct InputUtil<'a> {
    input_manager: &'a InputManager,
    config: &'a Config,
}

impl<'a> InputUtil<'a> {
    pub fn new(game_io: &'a GameIO) -> Self {
        Self {
            input_manager: game_io.input(),
            config: &game_io.resource::<Globals>().unwrap().config,
        }
    }

    pub fn latest_input(&self) -> Option<Input> {
        let latest_key = self.input_manager.latest_key();
        let latest_button = self.input_manager.latest_button();

        latest_key
            .and_then(|key| Self::input_from_binding(&self.config.key_bindings, key))
            .or_else(|| {
                latest_button
                    .and_then(|key| Self::input_from_binding(&self.config.controller_bindings, key))
            })
    }

    fn input_from_binding<K: std::cmp::PartialEq>(
        bindings: &HashMap<Input, Vec<K>>,
        key: K,
    ) -> Option<Input> {
        bindings
            .iter()
            .find(|(_, binded)| binded.contains(&key))
            .map(|(input, _)| *input)
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
        let config = &self.config;
        let input_manager = self.input_manager;

        if let Some(keys) = config.key_bindings.get(&input) {
            let is_down = keys.iter().any(|key| input_manager.is_key_down(*key));

            if is_down {
                return true;
            }
        }

        if let Some(buttons) = config.controller_bindings.get(&input) {
            let is_down = buttons
                .iter()
                .any(|button| input_manager.is_button_down(config.controller_index, *button));

            if is_down {
                return true;
            }
        }

        false
    }

    pub fn was_just_pressed(&self, input: Input) -> bool {
        let already_down = self.any(
            input,
            |key| {
                !self.input_manager.was_key_just_pressed(key) && self.input_manager.is_key_down(key)
            },
            |index, button| {
                !self.input_manager.was_button_just_pressed(index, button)
                    && self.input_manager.is_button_down(index, button)
            },
        );

        if already_down {
            // handle multiple bindings to the same input
            // prevent was_just_pressed from activating if other bindings are already pressed
            return false;
        }

        self.any(
            input,
            |key| self.input_manager.was_key_just_pressed(key),
            |index, button| self.input_manager.was_button_just_pressed(index, button),
        )
    }

    pub fn was_released(&self, input: Input) -> bool {
        // handle multiple bindings to the same input, nothing should still be down
        if self.is_down(input) {
            return false;
        }

        self.any(
            input,
            |key| self.input_manager.was_key_released(key),
            |index, button| self.input_manager.was_button_released(index, button),
        )
    }

    fn any(
        &self,
        input: Input,
        key_callback: impl Fn(Key) -> bool,
        button_callback: impl Fn(usize, Button) -> bool,
    ) -> bool {
        let config = &self.config;

        if let Some(keys) = config.key_bindings.get(&input) {
            if keys.iter().any(|key| key_callback(*key)) {
                return true;
            }
        }

        if let Some(buttons) = config.controller_bindings.get(&input) {
            if buttons
                .iter()
                .any(|button| button_callback(config.controller_index, *button))
            {
                return true;
            }
        }

        false
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
