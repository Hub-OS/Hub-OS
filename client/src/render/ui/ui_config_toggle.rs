use super::{FontStyle, TextStyle, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::{RefCell, RefMut};
use std::rc::Rc;

pub struct UiConfigToggle {
    name: &'static str,
    value: bool,
    config: Rc<RefCell<Config>>,
    callback: Box<dyn Fn(&mut GameIO, RefMut<Config>) -> bool>,
}

impl UiConfigToggle {
    pub fn new(
        name: &'static str,
        value: bool,
        config: Rc<RefCell<Config>>,
        callback: impl Fn(&mut GameIO, RefMut<Config>) -> bool + 'static,
    ) -> Self {
        Self {
            name,
            config,
            value,
            callback: Box::new(callback),
        }
    }
}

impl UiNode for UiConfigToggle {
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
        let text = if self.value { "true" } else { "false" };
        let metrics = text_style.measure(text);
        text_style.bounds.x += bounds.width - metrics.size.x - 1.0;

        text_style.draw(game_io, sprite_queue, text);
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        if !focused {
            return;
        }

        let input_util = InputUtil::new(game_io);

        let confirmed = input_util.was_just_pressed(Input::Confirm)
            || input_util.was_just_pressed(Input::Left)
            || input_util.was_just_pressed(Input::Right);

        if !confirmed {
            return;
        }

        let original_value = self.value;
        self.value = (self.callback)(game_io, self.config.borrow_mut());

        let globals = game_io.resource::<Globals>().unwrap();

        if self.value == original_value {
            // toggle failed
            globals.audio.play_sound(&globals.cursor_error_sfx);
        } else {
            globals.audio.play_sound(&globals.cursor_select_sfx);
        }
    }
}
