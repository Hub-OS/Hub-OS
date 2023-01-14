use super::{FontStyle, TextStyle, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

pub struct UiConfigBinding {
    binds_keyboard: bool,
    input: Input,
    config: Rc<RefCell<Config>>,
    binding: bool,
}

impl UiConfigBinding {
    pub fn new_keyboard(input: Input, config: Rc<RefCell<Config>>) -> Self {
        Self {
            binds_keyboard: true,
            input,
            config,
            binding: false,
        }
    }

    pub fn new_controller(input: Input, config: Rc<RefCell<Config>>) -> Self {
        Self {
            binds_keyboard: false,
            input,
            config,
            binding: false,
        }
    }

    fn bound_text(&self) -> Option<&'static str> {
        let config = self.config.borrow();

        if self.binds_keyboard {
            let key = *config.key_bindings.get(&self.input)?;

            Some(key.into())
        } else {
            let button = *config.controller_bindings.get(&self.input)?;

            let text = match button {
                Button::LeftShoulder => "L-Shldr",
                Button::LeftTrigger => "L-Trgger",
                Button::RightShoulder => "R-Shldr",
                Button::RightTrigger => "R-Trgger",
                Button::LeftStick => "L-Stick",
                Button::RightStick => "R-Stick",
                Button::LeftStickUp => "L-Up",
                Button::LeftStickDown => "L-Down",
                Button::LeftStickLeft => "L-Left",
                Button::LeftStickRight => "L-Right",
                Button::RightStickUp => "R-Up",
                Button::RightStickDown => "R-Down",
                Button::RightStickLeft => "R-Left",
                Button::RightStickRight => "R-Right",
                button => button.into(),
            };

            Some(text)
        }
    }
}

impl UiNode for UiConfigBinding {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        let bound_text = self.bound_text();

        if Input::REQUIRED_INPUTS.contains(&self.input) && bound_text.is_none() {
            // display unbound required inputs in red
            text_style.color = Color::ORANGE;
        }

        // draw input name
        let text: &'static str = match self.input {
            Input::AdvanceFrame => "AdvFrame",
            Input::RewindFrame => "RewFrame",
            input => input.into(),
        };

        text_style.draw(game_io, sprite_queue, text);

        // draw binding
        let text: &'static str = if self.binding {
            "..."
        } else {
            bound_text.unwrap_or_default()
        };

        let metrics = text_style.measure(text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, text);
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn focusable(&self) -> bool {
        true
    }

    fn is_locking_focus(&self) -> bool {
        self.binding
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        if !focused {
            return;
        }

        if !self.binding {
            let input_util = InputUtil::new(game_io);

            // erase binding
            if input_util.was_just_pressed(Input::Option) {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.cursor_cancel_sfx);

                let mut config = self.config.borrow_mut();

                if self.binds_keyboard {
                    config.key_bindings.remove(&self.input);
                } else {
                    config.controller_bindings.remove(&self.input);
                }
            }

            // begin new binding
            if input_util.was_just_pressed(Input::Confirm) {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.cursor_select_sfx);

                self.binding = true;
            }
            return;
        }

        if self.binds_keyboard {
            if let Some(key) = game_io.input().latest_key() {
                let mut config = self.config.borrow_mut();
                Self::bind(&mut config.key_bindings, self.input, key);
                self.binding = false;
            }
        } else {
            if game_io.input().latest_key().is_some() {
                self.binding = false;
            }

            if let Some(button) = game_io.input().latest_button() {
                let mut config = self.config.borrow_mut();
                Self::bind(&mut config.controller_bindings, self.input, button);
                self.binding = false;
            }
        }
    }
}

impl UiConfigBinding {
    fn bind<V: std::cmp::PartialEq + Clone>(
        bindings: &mut HashMap<Input, V>,
        input: Input,
        value: V,
    ) {
        if Input::REQUIRED_INPUTS.contains(&input) {
            // unbind overlapping input
            if let Some(input) = bindings.iter().find(|(_, v)| **v == value).map(|(k, _)| *k) {
                bindings.remove(&input);
            }
        }

        bindings.insert(input, value);
    }
}
