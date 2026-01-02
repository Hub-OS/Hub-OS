use super::ui_config_cycle_arrows::UiConfigCycleArrows;
use super::{FontName, OverflowTextScroller, TextStyle, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::{RefCell, RefMut};
use std::rc::Rc;

struct LockedState<T> {
    original_value: T,
    arrows: UiConfigCycleArrows,
}

pub struct UiConfigDynamicCycle<T> {
    label: String,
    value: T,
    value_text: String,
    text_scroller: OverflowTextScroller,
    config: Rc<RefCell<Config>>,
    text_callback: Box<dyn Fn(&mut GameIO, &T) -> String>,
    cycle_callback: Box<dyn Fn(&mut GameIO, &T, bool) -> T>,
    callback: Box<dyn Fn(&mut GameIO, RefMut<Config>, &T)>,
    locked_state: Option<Box<LockedState<T>>>,
}

impl<T> UiConfigDynamicCycle<T> {
    pub fn new(
        game_io: &mut GameIO,
        label_translation_key: &'static str,
        value: T,
        config: Rc<RefCell<Config>>,
        text_callback: impl Fn(&mut GameIO, &T) -> String + 'static,
        // the bool is for pressing right, false = left
        cycle_callback: impl Fn(&mut GameIO, &T, bool) -> T + 'static,
        callback: impl Fn(&mut GameIO, RefMut<Config>, &T) + 'static,
    ) -> Self {
        let value_text = text_callback(game_io, &value);

        let globals = Globals::from_resources(game_io);

        Self {
            label: globals.translate(label_translation_key),
            value,
            value_text,
            text_scroller: OverflowTextScroller::new(),
            config,
            text_callback: Box::new(text_callback),
            cycle_callback: Box::new(cycle_callback),
            callback: Box::new(callback),
            locked_state: None,
        }
    }
}

impl<T> UiConfigDynamicCycle<T> {
    pub fn cycle_slice(slice: &[T], cycle_right: bool, eq_test: impl Fn(&T) -> bool) -> Option<&T> {
        if cycle_right {
            let value_after_previous = slice.iter().skip_while(|value| !eq_test(*value)).nth(1);

            value_after_previous.or_else(|| slice.first())
        } else {
            let name_before_previous = slice
                .iter()
                .rev()
                .skip_while(|value| !eq_test(*value))
                .nth(1);

            name_before_previous.or_else(|| slice.last())
        }
    }
}

impl<T: PartialEq + Clone> UiNode for UiConfigDynamicCycle<T> {
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
        text_style.draw(game_io, sprite_queue, &self.label);

        // draw value
        let range = self.text_scroller.text_range(&self.value_text);
        let text = &self.value_text[range];

        let metrics = text_style.measure(text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, text);

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
        if !focused {
            self.text_scroller.reset();
            return;
        }

        self.text_scroller.update();

        let input_util = InputUtil::new(game_io);
        let left = input_util.was_just_pressed(Input::Left);
        let right = input_util.was_just_pressed(Input::Right);
        let confirm = input_util.was_just_pressed(Input::Confirm);

        if !self.is_locking_focus() {
            if confirm {
                // focus, play sfx, update, and continue
                let globals = Globals::from_resources(game_io);
                globals.audio.play_sound(&globals.sfx.cursor_select);

                self.locked_state = Some(Box::new(LockedState {
                    original_value: self.value.clone(),
                    arrows: UiConfigCycleArrows::new(game_io),
                }));
            }
        } else if confirm {
            // unfocus and play sfx
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.locked_state = None;
        } else if input_util.was_just_pressed(Input::Cancel) {
            // unfocus, undo, and play sfx
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            if let Some(locked_state) = self.locked_state.take() {
                self.value = locked_state.original_value;
                self.value_text = (self.text_callback)(game_io, &self.value);
                (self.callback)(game_io, self.config.borrow_mut(), &self.value);
            }
        }

        // update arrows, also tests if we're locking input
        let Some(locked_state) = &mut self.locked_state else {
            return;
        };

        locked_state.arrows.update();

        // test input
        let pressing_single_direction = left ^ right;

        if !pressing_single_direction {
            return;
        }

        let new_value = (self.cycle_callback)(game_io, &self.value, right);
        let value_changed = self.value != new_value;

        self.value_text = (self.text_callback)(game_io, &new_value);
        self.value = new_value;

        if !value_changed {
            return;
        }

        (self.callback)(game_io, self.config.borrow_mut(), &self.value);

        self.text_scroller.reset();
        let globals = Globals::from_resources(game_io);
        globals.audio.play_sound(&globals.sfx.cursor_move);
    }
}
