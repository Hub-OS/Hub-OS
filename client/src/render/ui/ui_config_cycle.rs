use super::{FontStyle, TextStyle, UiNode};
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::saves::Config;
use framework::prelude::*;
use std::cell::{RefCell, RefMut};
use std::rc::Rc;

pub struct UiConfigCycle<T> {
    name: &'static str,
    selection: usize,
    options: Vec<(String, T)>,
    config: Rc<RefCell<Config>>,
    callback: Box<dyn Fn(&mut GameIO, RefMut<Config>, T)>,
}

impl<T: Copy + PartialEq> UiConfigCycle<T> {
    pub fn new(
        name: &'static str,
        value: T,
        config: Rc<RefCell<Config>>,
        options: &[(&str, T)],
        callback: impl Fn(&mut GameIO, RefMut<Config>, T) + 'static,
    ) -> Self {
        Self {
            name,
            selection: options
                .iter()
                .position(|(_, v)| *v == value)
                .unwrap_or_default(),
            options: options
                .iter()
                .map(|(name, value)| (name.to_string(), *value))
                .collect(),
            config,
            callback: Box::new(callback),
        }
    }
}

impl<T: Copy> UiNode for UiConfigCycle<T> {
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
        let text = &self.options[self.selection].0;
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

        let left = input_util.was_just_pressed(Input::Left);
        let right = input_util.was_just_pressed(Input::Confirm)
            || input_util.was_just_pressed(Input::Right);

        if !left && !right {
            return;
        }

        let original_selection = self.selection;

        if left {
            if self.selection == 0 {
                self.selection = self.options.len() - 1
            } else {
                self.selection -= 1;
            }
        }

        if right {
            self.selection += 1;
            self.selection = (self.selection)
                .checked_rem(self.options.len())
                .unwrap_or_default();
        }

        if self.selection == original_selection {
            return;
        }

        let value = self.options[self.selection].1;
        (self.callback)(game_io, self.config.borrow_mut(), value);

        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.play_sound(&globals.sfx.cursor_move);
    }
}
