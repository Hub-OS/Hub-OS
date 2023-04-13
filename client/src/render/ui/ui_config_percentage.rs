use super::{FontStyle, TextStyle, UiInputTracker, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::{RefCell, RefMut};
use std::rc::Rc;

pub struct UiConfigPercentage {
    name: &'static str,
    value: u8,
    value_text: String,
    lower_bound: u8,
    upper_bound: u8,
    auditory_feedback: bool,
    locking_focus: bool,
    ui_input_tracker: UiInputTracker,
    config: Rc<RefCell<Config>>,
    callback: Box<dyn Fn(&mut GameIO, RefMut<Config>, u8)>,
}

impl UiConfigPercentage {
    pub fn new(
        name: &'static str,
        value: u8,
        config: Rc<RefCell<Config>>,
        callback: impl Fn(&mut GameIO, RefMut<Config>, u8) + 'static,
    ) -> Self {
        Self {
            name,
            value,
            value_text: Self::generate_value_text(value),
            lower_bound: 0,
            upper_bound: 100,
            auditory_feedback: true,
            locking_focus: false,
            ui_input_tracker: UiInputTracker::new(),
            config,
            callback: Box::new(callback),
        }
    }

    pub fn with_upper_bound(mut self, value: u8) -> Self {
        self.upper_bound = value;
        self
    }

    pub fn with_lower_bound(mut self, value: u8) -> Self {
        self.lower_bound = value;
        self
    }

    pub fn with_auditory_feedback(mut self, enabled: bool) -> Self {
        self.auditory_feedback = enabled;
        self
    }

    fn generate_value_text(level: u8) -> String {
        format!("{level}%")
    }
}

impl UiNode for UiConfigPercentage {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        // draw name
        text_style.draw(game_io, sprite_queue, self.name);

        // draw value
        let metrics = text_style.measure(&self.value_text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, &self.value_text);
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn is_locking_focus(&self) -> bool {
        self.locking_focus
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        self.ui_input_tracker.update(game_io);

        if !focused {
            return;
        }

        let input_util = InputUtil::new(game_io);

        let confirm = input_util.was_just_pressed(Input::Confirm);
        let left = self.ui_input_tracker.is_active(Input::Left);
        let right = self.ui_input_tracker.is_active(Input::Right);

        if !self.locking_focus {
            if left || right || confirm {
                // focus, play sfx, update, and continue
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.sfx.cursor_select);

                self.locking_focus = true;
            } else {
                // no focus
                return;
            }
        } else if confirm || input_util.was_just_pressed(Input::Cancel) {
            // unfocus and play sfx
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.locking_focus = false;
            return;
        }

        if !self.locking_focus {
            // can't change value unless the input is selected
            return;
        }

        let up = self.ui_input_tracker.is_active(Input::Up);
        let down = self.ui_input_tracker.is_active(Input::Down);

        // adjusting value
        let original_level = self.value;

        // nudge by 1
        if (left || down) && self.value > self.lower_bound {
            self.value -= 1;
        }

        if (right || up) && self.value < self.upper_bound {
            self.value += 1;
        }

        // nudge by 10
        if self.ui_input_tracker.is_active(Input::ShoulderL) {
            if self.value >= self.lower_bound + 10 {
                self.value -= 10;
            } else {
                self.value = self.lower_bound;
            }
        }

        if self.ui_input_tracker.is_active(Input::ShoulderR) {
            if self.value <= self.upper_bound - 10 {
                self.value += 10;
            } else {
                self.value = self.upper_bound;
            }
        }

        if self.value == original_level {
            // no change, no need to update anything
            return;
        }

        // update visual
        self.value_text = Self::generate_value_text(self.value);

        (self.callback)(game_io, self.config.borrow_mut(), self.value);

        // play sfx after callback in case the sfx volume is adjusted
        if self.auditory_feedback {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }
    }
}
