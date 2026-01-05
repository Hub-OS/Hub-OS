use super::{FontName, TextStyle, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::{RefCell, RefMut};
use std::rc::Rc;

pub struct UiConfigToggle {
    label: String,
    value: bool,
    config: Rc<RefCell<Config>>,
    callback: Box<dyn Fn(&mut GameIO, RefMut<Config>) -> bool>,
}

impl UiConfigToggle {
    pub fn new(
        game_io: &GameIO,
        label_translation_key: &'static str,
        value: bool,
        config: Rc<RefCell<Config>>,
        callback: impl Fn(&mut GameIO, RefMut<Config>) -> bool + 'static,
    ) -> Self {
        let globals = Globals::from_resources(game_io);

        Self {
            label: globals.translate(label_translation_key),
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
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(bounds.position());

        // draw name
        text_style.draw(game_io, sprite_queue, &self.label);

        // draw value
        let translation_key = if self.value {
            "config-value-true"
        } else {
            "config-value-false"
        };

        let text = &Globals::from_resources(game_io).translate(translation_key);
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

        if !input_util.was_just_pressed(Input::Confirm) {
            return;
        }

        let original_value = self.value;
        self.value = (self.callback)(game_io, self.config.borrow_mut());

        let globals = Globals::from_resources(game_io);

        if self.value == original_value {
            // toggle failed
            globals.audio.play_sound(&globals.sfx.cursor_error);
        } else {
            globals.audio.play_sound(&globals.sfx.cursor_select);
        }
    }
}
