use super::{FontStyle, OverflowTextScroller, TextStyle, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::{RefCell, RefMut};
use std::rc::Rc;

pub struct UiConfigDynamicCycle<T> {
    name: &'static str,
    value: Option<T>,
    value_text: String,
    text_scroller: OverflowTextScroller,
    config: Rc<RefCell<Config>>,
    text_callback: Box<dyn Fn(&mut GameIO, &T) -> String>,
    callback: Box<dyn Fn(&mut GameIO, RefMut<Config>, &T, bool) -> T>,
}

impl<T> UiConfigDynamicCycle<T> {
    pub fn new(
        game_io: &mut GameIO,
        name: &'static str,
        value: T,
        config: Rc<RefCell<Config>>,
        text_callback: impl Fn(&mut GameIO, &T) -> String + 'static,
        // the bool is for pressing right, false = left
        callback: impl Fn(&mut GameIO, RefMut<Config>, &T, bool) -> T + 'static,
    ) -> Self {
        let value_text = text_callback(game_io, &value);

        Self {
            name,
            value: Some(value),
            value_text,
            text_scroller: OverflowTextScroller::new(),
            config,
            text_callback: Box::new(text_callback),
            callback: Box::new(callback),
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

impl<T: PartialEq> UiNode for UiConfigDynamicCycle<T> {
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
        let range = self.text_scroller.text_range(&self.value_text);
        let text = &self.value_text[range];

        let metrics = text_style.measure(text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, text);
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        if !focused {
            self.text_scroller.reset();
            return;
        }

        self.text_scroller.update();

        let input_util = InputUtil::new(game_io);

        let left = input_util.was_just_pressed(Input::Left);
        let right = input_util.was_just_pressed(Input::Confirm)
            || input_util.was_just_pressed(Input::Right);

        let pressing_single_direction = left ^ right;

        if !pressing_single_direction {
            return;
        }

        let previous_value = self.value.take().unwrap();
        let new_value = (self.callback)(game_io, self.config.borrow_mut(), &previous_value, right);
        let value_changed = previous_value != new_value;

        self.value_text = (self.text_callback)(game_io, &new_value);
        self.value = Some(new_value);

        if !value_changed {
            return;
        }

        self.text_scroller.reset();
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.cursor_move_sfx);
    }
}
