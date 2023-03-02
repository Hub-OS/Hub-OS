use crate::render::*;
use crate::resources::Input;

#[derive(Default, Clone)]
pub struct PlayerInput {
    previous_input: Vec<Input>,
    pressed_input: Vec<Input>,
    held_navigation_input: Vec<Input>,
    navigation_held_duration: FrameTime,
}

impl PlayerInput {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn is_active(&self, input: Input) -> bool {
        const LONG_DELAY: FrameTime = 15;
        const SHORT_DELAY: FrameTime = 5;

        if !self.pressed_input.contains(&input) {
            return false;
        }

        if !Input::REPEATABLE.contains(&input) {
            return self.was_just_pressed(input);
        }

        let duration = self.navigation_held_duration;

        if duration < LONG_DELAY {
            duration % LONG_DELAY == 0
        } else {
            (duration - LONG_DELAY) % SHORT_DELAY == 0
        }
    }

    pub fn is_down(&self, input: Input) -> bool {
        self.pressed_input.contains(&input)
    }

    pub fn was_just_pressed(&self, input: Input) -> bool {
        !self.previous_input.contains(&input) && self.pressed_input.contains(&input)
    }

    pub fn was_released(&self, input: Input) -> bool {
        self.previous_input.contains(&input) && !self.pressed_input.contains(&input)
    }

    pub fn matches(&self, pressed_input: &[Input]) -> bool {
        self.pressed_input == pressed_input
    }

    pub fn set_pressed_input(&mut self, inputs: Vec<Input>) {
        self.pressed_input = inputs;
    }

    pub fn simulate_input(&mut self, input: Input, pressed: bool) {
        let position = self.pressed_input.iter().position(|v| *v == input);

        if pressed {
            if position.is_none() {
                self.pressed_input.push(input);
            }
        } else if let Some(i) = position {
            self.pressed_input.remove(i);
        }
    }

    pub fn flush(&mut self) {
        // copy previous state
        self.previous_input = self.pressed_input.clone();

        let navigation_held = self
            .previous_input
            .iter()
            .any(|input| Input::REPEATABLE.contains(input));

        if navigation_held {
            self.navigation_held_duration += 1;
        } else {
            self.navigation_held_duration = 0;
        }
    }
}
