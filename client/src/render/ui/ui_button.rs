use super::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct UiButton<'a, T> {
    content: T,
    activate_callback: Box<dyn Fn() + 'a>,
    focus_callback: Option<Box<dyn Fn()>>,
    was_focused: bool,
}

impl UiButton<'_, Text> {
    pub fn new_translated(game_io: &GameIO, font: FontName, text_key: &str) -> Self {
        let globals = Globals::from_resources(game_io);

        Self::new(
            Text::new(game_io, font)
                .with_string(globals.translate(text_key))
                .with_shadow_color(TEXT_DARK_SHADOW_COLOR),
        )
    }
}

impl<'a, T> UiButton<'a, T> {
    pub fn new(content: T) -> Self {
        Self {
            content,
            activate_callback: Box::new(|| {}),
            focus_callback: None,
            was_focused: false,
        }
    }

    pub fn on_activate(mut self, callback: impl Fn() + 'a) -> Self {
        self.activate_callback = Box::new(callback);
        self
    }

    pub fn on_focus(mut self, callback: impl Fn() + 'static) -> Self {
        self.focus_callback = Some(Box::new(callback));
        self
    }
}

impl<T> UiNode for UiButton<'_, T>
where
    T: UiNode,
{
    fn focusable(&self) -> bool {
        true
    }

    fn update(&mut self, game_io: &mut GameIO, _bounds: Rect, focused: bool) {
        if !focused {
            self.was_focused = false;
            return;
        }

        if !self.was_focused {
            self.was_focused = true;

            if let Some(callback) = &self.focus_callback {
                callback();
            }
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Confirm) {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);

            (self.activate_callback)();
        }
    }

    fn measure_ui_size(&mut self, game_io: &GameIO) -> Vec2 {
        self.content.measure_ui_size(game_io)
    }

    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        self.content.draw_bounded(game_io, sprite_queue, bounds)
    }
}
