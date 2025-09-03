use packets::{NetplayBufferItem, NetplaySignal};

use crate::render::*;
use crate::resources::Input;

#[derive(Default, Clone)]
pub struct PlayerInput {
    previous_input: Vec<Input>,
    pressed_input: Vec<Input>,
    held_navigation_input: Vec<Input>,
    navigation_held_duration: FrameTime,
    signals: Vec<NetplaySignal>,
    input_delay: u8,
    disconnected: bool,
    fleeing: bool,
}

impl PlayerInput {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn input_delay(&self) -> usize {
        self.input_delay as _
    }

    pub fn set_input_delay(&mut self, delay: usize) {
        self.input_delay = delay as _;
    }

    pub fn pulsed(&self, input: Input) -> bool {
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

    pub fn disconnected(&self) -> bool {
        self.disconnected
    }

    pub fn fleeing(&self) -> bool {
        self.fleeing
    }

    pub fn has_signal(&self, signal: NetplaySignal) -> bool {
        self.signals.contains(&signal)
    }

    pub fn matches(&self, data: &NetplayBufferItem) -> bool {
        data.signals.is_empty() && self.pressed_input == data.pressed
    }

    pub fn load_data(&mut self, data: NetplayBufferItem) {
        self.pressed_input = data.pressed;
        self.signals = data.signals;

        if self.has_signal(NetplaySignal::AttemptingFlee) {
            self.fleeing = true;
        }

        if self.has_signal(NetplaySignal::Disconnect) {
            self.disconnected = true;
        }

        if self.has_signal(NetplaySignal::CompletedFlee) || self.disconnected {
            self.fleeing = false;
        }
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
        self.previous_input.clone_from(&self.pressed_input);
        self.signals.clear();

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
