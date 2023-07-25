use framework::prelude::Button;

// controlled by mobile_overlay.rs
#[derive(Default)]
pub struct EmulatedInput {
    previous_buttons: Vec<Button>,
    pressed_buttons: Vec<Button>,
    latest_button: Option<Button>,
}

impl EmulatedInput {
    pub fn flush(&mut self) {
        std::mem::swap(&mut self.previous_buttons, &mut self.pressed_buttons);
        self.pressed_buttons.clear();
        self.latest_button = None;
    }

    pub fn emulate_button(&mut self, button: Button) {
        self.pressed_buttons.push(button);

        if !self.previous_buttons.contains(&button) {
            self.latest_button = Some(button);
        }
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
}
