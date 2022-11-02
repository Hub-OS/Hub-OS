pub use strum::EnumString;

#[derive(EnumString, Debug, PartialEq, Eq, Copy, Clone)]
pub enum AnalogAxis {
    DPadX,
    DPadY,
    LeftStickX,
    LeftStickY,
    RightStickX,
    RightStickY,
    LeftTrigger,
    RightTrigger,
}

impl AnalogAxis {
    pub fn is_x_axis(&self) -> bool {
        matches!(
            self,
            AnalogAxis::DPadX | AnalogAxis::LeftStickX | AnalogAxis::RightStickX
        )
    }

    pub fn is_y_axis(&self) -> bool {
        matches!(
            self,
            AnalogAxis::DPadY | AnalogAxis::LeftStickY | AnalogAxis::RightStickY
        )
    }
}

#[derive(EnumString, Debug, PartialEq, Eq, Copy, Clone)]
pub enum Button {
    Meta,
    Start,
    Select,
    A,
    B,
    X,
    Y,
    C,
    Z,
    LeftShoulder,
    LeftTrigger,
    RightShoulder,
    RightTrigger,
    LeftStick,
    RightStick,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    LeftStickUp,
    LeftStickDown,
    LeftStickLeft,
    LeftStickRight,
    RightStickUp,
    RightStickDown,
    RightStickLeft,
    RightStickRight,
    Paddle1,
    Paddle2,
    Paddle3,
    Paddle4,
}

use crate::prelude::RumblePack;
use std::time::Duration;

pub struct GameController {
    id: usize,
    rumble_pack: RumblePack,
    latest_button: Option<Button>,
    previous_buttons: Vec<Button>,
    pressed_buttons: Vec<Button>,
    left_stick_x: f32,
    left_stick_y: f32,
    right_stick_x: f32,
    right_stick_y: f32,
    left_trigger: f32,
    right_trigger: f32,
}

impl GameController {
    pub(super) fn new(id: usize, rumble_pack: RumblePack) -> Self {
        Self {
            id,
            rumble_pack,
            latest_button: None,
            previous_buttons: Vec::new(),
            pressed_buttons: Vec::new(),
            left_stick_x: 0.0,
            left_stick_y: 0.0,
            right_stick_x: 0.0,
            right_stick_y: 0.0,
            left_trigger: 0.0,
            right_trigger: 0.0,
        }
    }

    pub fn id(&self) -> usize {
        self.id
    }

    pub fn rumble(&self, weak: f32, strong: f32, duration: Duration) {
        self.rumble_pack.rumble(weak, strong, duration);
    }

    pub fn latest_button(&self) -> Option<Button> {
        self.latest_button
    }

    pub fn is_button_down(&self, button: Button) -> bool {
        self.pressed_buttons.contains(&button)
    }

    pub fn was_button_just_pressed(&self, button: Button) -> bool {
        !self.previous_buttons.contains(&button) && self.pressed_buttons.contains(&button)
    }

    pub fn was_button_released(&self, button: Button) -> bool {
        self.previous_buttons.contains(&button) && !self.pressed_buttons.contains(&button)
    }

    pub fn buttons_as_axis(&self, negative: Button, positive: Button) -> f32 {
        let mut value = 0.0;

        if self.pressed_buttons.contains(&negative) {
            value -= 1.0;
        }

        if self.pressed_buttons.contains(&positive) {
            value += 1.0;
        }

        value
    }

    pub fn axis(&self, axis: AnalogAxis) -> f32 {
        match axis {
            AnalogAxis::LeftTrigger => self.left_trigger,
            AnalogAxis::RightTrigger => self.right_trigger,
            AnalogAxis::DPadX => self.buttons_as_axis(Button::DPadLeft, Button::DPadRight),
            AnalogAxis::DPadY => self.buttons_as_axis(Button::DPadDown, Button::DPadUp),
            AnalogAxis::LeftStickX => self.left_stick_x,
            AnalogAxis::LeftStickY => self.left_stick_y,
            AnalogAxis::RightStickX => self.right_stick_x,
            AnalogAxis::RightStickY => self.right_stick_y,
        }
    }

    pub(crate) fn simulate_button_press(&mut self, button: Button) {
        if !self.pressed_buttons.contains(&button) {
            self.latest_button = Some(button);
            self.pressed_buttons.push(button);
        }
    }

    pub(crate) fn simulate_button_release(&mut self, button: Button) {
        if let Some(index) = self.pressed_buttons.iter().position(|v| *v == button) {
            self.pressed_buttons.swap_remove(index);
        }
    }

    pub(crate) fn simulate_axis_movement(&mut self, axis: AnalogAxis, value: f32) {
        match axis {
            AnalogAxis::DPadX | AnalogAxis::DPadY => {}
            AnalogAxis::LeftTrigger => self.left_trigger = value,
            AnalogAxis::RightTrigger => self.right_trigger = value,
            AnalogAxis::LeftStickX => {
                self.left_stick_x = value;

                if value < 0.0 {
                    self.simulate_button_press(Button::LeftStickLeft);
                } else if value > 0.0 {
                    self.simulate_button_press(Button::LeftStickRight);
                } else {
                    self.simulate_button_release(Button::LeftStickLeft);
                    self.simulate_button_release(Button::LeftStickRight);
                }
            }
            AnalogAxis::LeftStickY => {
                self.left_stick_y = value;

                if value < 0.0 {
                    self.simulate_button_press(Button::LeftStickDown);
                } else if value > 0.0 {
                    self.simulate_button_press(Button::LeftStickUp);
                } else {
                    self.simulate_button_release(Button::LeftStickDown);
                    self.simulate_button_release(Button::LeftStickUp);
                }
            }
            AnalogAxis::RightStickX => {
                self.right_stick_x = value;

                if value < 0.0 {
                    self.simulate_button_press(Button::RightStickLeft);
                } else if value > 0.0 {
                    self.simulate_button_press(Button::RightStickRight);
                } else {
                    self.simulate_button_release(Button::RightStickLeft);
                    self.simulate_button_release(Button::RightStickRight);
                }
            }
            AnalogAxis::RightStickY => {
                self.right_stick_y = value;

                if value < 0.0 {
                    self.simulate_button_press(Button::RightStickDown);
                } else if value > 0.0 {
                    self.simulate_button_press(Button::RightStickUp);
                } else {
                    self.simulate_button_release(Button::RightStickDown);
                    self.simulate_button_release(Button::RightStickUp);
                }
            }
        }
    }

    pub(crate) fn flush(&mut self) {
        self.previous_buttons = self.pressed_buttons.clone();
        self.latest_button = None;
    }
}
