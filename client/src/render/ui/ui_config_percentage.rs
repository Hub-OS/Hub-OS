use super::ui_config_cycle_arrows::UiConfigCycleArrows;
use super::{FontName, TextStyle, UiInputTracker, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::{RefCell, RefMut};
use std::rc::Rc;

struct LockedState {
    original_value: u8,
    arrows: UiConfigCycleArrows,
}

pub struct UiConfigPercentage {
    name: &'static str,
    value: u8,
    value_text: String,
    lower_bound: u8,
    upper_bound: u8,
    auditory_feedback: bool,
    ui_input_tracker: UiInputTracker,
    config: Rc<RefCell<Config>>,
    callback: Box<dyn Fn(&mut GameIO, RefMut<Config>, u8)>,
    locked_state: Option<Box<LockedState>>,
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
            ui_input_tracker: UiInputTracker::new(),
            config,
            callback: Box::new(callback),
            locked_state: None,
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
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        // draw name
        text_style.draw(game_io, sprite_queue, self.name);

        // draw value
        let metrics = text_style.measure(&self.value_text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, &self.value_text);

        if let Some(locked_state) = &mut self.locked_state {
            let mut bounds = bounds;
            bounds.height = metrics.size.y;
            locked_state.arrows.draw(sprite_queue, bounds);
        }
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn is_locking_focus(&self) -> bool {
        self.locked_state.is_some()
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        self.ui_input_tracker.update(game_io);

        if !focused {
            return;
        }

        let input_util = InputUtil::new(game_io);
        let confirm = input_util.was_just_pressed(Input::Confirm);

        if !self.is_locking_focus() {
            if confirm {
                // focus, play sfx, update, and continue
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.sfx.cursor_select);

                self.locked_state = Some(Box::new(LockedState {
                    original_value: self.value,
                    arrows: UiConfigCycleArrows::new(game_io),
                }));
            }
        } else if confirm {
            // unfocus and play sfx
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.locked_state = None;
        } else if input_util.was_just_pressed(Input::Cancel) {
            // unfocus, undo, and play sfx
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            if let Some(locked_state) = self.locked_state.take() {
                self.value = locked_state.original_value;
                self.value_text = Self::generate_value_text(self.value);
                (self.callback)(game_io, self.config.borrow_mut(), self.value);
            }
        }

        // update arrows, also tests if we're locking input
        let Some(locked_state) = &mut self.locked_state else {
            return;
        };

        locked_state.arrows.update();

        // test input
        let left = self.ui_input_tracker.is_active(Input::Left);
        let right = self.ui_input_tracker.is_active(Input::Right);
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
