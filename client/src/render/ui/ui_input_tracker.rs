use crate::render::*;
use crate::resources::*;
use framework::prelude::GameIO;

pub struct UiInputTracker {
    held_navigation_inputs: Vec<Input>,
    navigation_held_duration: FrameTime,
    just_pressed_inputs: Vec<Input>,
}

impl UiInputTracker {
    pub fn new() -> Self {
        Self {
            held_navigation_inputs: Vec::new(),
            just_pressed_inputs: Vec::new(),
            navigation_held_duration: 0,
        }
    }

    pub fn pulsed(&self, input: Input) -> bool {
        const LONG_DELAY: FrameTime = 15;
        const SHORT_DELAY: FrameTime = 5;

        if self.just_pressed_inputs.contains(&input) {
            return true;
        }

        if !self.held_navigation_inputs.contains(&input) {
            return false;
        }

        let duration = self.navigation_held_duration;

        if duration < LONG_DELAY {
            duration % LONG_DELAY == 0
        } else {
            (duration - LONG_DELAY) % SHORT_DELAY == 0
        }
    }

    pub fn input_as_axis(&self, negative: Input, positive: Input) -> f32 {
        let mut value = 0.0;

        if self.pulsed(negative) {
            value -= 1.0;
        }

        if self.pulsed(positive) {
            value += 1.0;
        }

        value
    }

    pub fn update(&mut self, game_io: &GameIO) {
        use strum::IntoEnumIterator;

        let input_util = InputUtil::new(game_io);

        // detect fresh input
        self.just_pressed_inputs.clear();

        for input in Input::iter() {
            if input_util.was_just_pressed(input) {
                self.just_pressed_inputs.push(input);
            }
        }

        // detect held navigation inputs
        self.held_navigation_inputs.clear();

        for input in Input::REPEATABLE {
            if input_util.is_down(input) {
                self.held_navigation_inputs.push(input);
            }
        }

        if self.held_navigation_inputs.is_empty() {
            self.navigation_held_duration = -1;
        } else {
            self.navigation_held_duration += 1;
        }
    }
}
