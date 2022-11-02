use crate::render::*;
use crate::resources::*;
use framework::prelude::GameIO;
use std::collections::HashSet;

pub struct UiInputTracker {
    held_inputs: HashSet<Input>,
    held_duration: FrameTime,
    held_button: Option<Input>,
}

impl UiInputTracker {
    pub fn new() -> Self {
        Self {
            held_inputs: HashSet::new(),
            held_duration: 0,
            held_button: None,
        }
    }

    pub fn is_active(&self, input: Input) -> bool {
        const LONG_DELAY: FrameTime = 15;
        const SHORT_DELAY: FrameTime = 5;

        if self.held_inputs.get(&input).is_none() {
            return false;
        }

        let duration = self.held_duration;

        if duration < LONG_DELAY {
            duration % LONG_DELAY == 0
        } else {
            (duration - LONG_DELAY) % SHORT_DELAY == 0
        }
    }

    pub fn input_as_axis(&self, negative: Input, positive: Input) -> f32 {
        let mut value = 0.0;

        if self.is_active(negative) {
            value -= 1.0;
        }

        if self.is_active(positive) {
            value += 1.0;
        }

        value
    }

    pub fn update(&mut self, game_io: &GameIO<Globals>) {
        use strum::IntoEnumIterator;

        let input_util = InputUtil::new(game_io);
        let mut nothing_held = true;

        for input in Input::iter() {
            if input_util.is_down(input) {
                nothing_held = false;
                self.held_inputs.insert(input);
            } else {
                self.held_inputs.remove(&input);
            }
        }

        if nothing_held {
            self.held_duration = -1;
        } else {
            self.held_duration += 1;
        }
    }
}
