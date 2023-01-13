use super::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct UiButton<'a> {
    text: Text,
    activate_callback: Box<dyn Fn() + 'a>,
    focus_callback: Option<Box<dyn Fn()>>,
    was_focused: bool,
}

impl<'a> UiButton<'a> {
    pub fn new(game_io: &GameIO, font_style: FontStyle, text: &str) -> Self {
        Self {
            text: Text::new(game_io, font_style)
                .with_str(text)
                .with_shadow_color(TEXT_DARK_SHADOW_COLOR),
            activate_callback: Box::new(|| {}),
            focus_callback: None,
            was_focused: false,
        }
    }

    pub fn with_text_style(mut self, style: TextStyle) -> Self {
        self.text.style = style;
        self
    }

    pub fn with_shadow_color(mut self, color: Color) -> Self {
        self.text.style.shadow_color = color;
        self
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

impl<'a> UiNode for UiButton<'a> {
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
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.cursor_select_sfx);

            (self.activate_callback)();
        }
    }

    fn measure_ui_size(&mut self, game_io: &GameIO) -> Vec2 {
        self.text.measure_ui_size(game_io)
    }

    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        self.text.draw_bounded(game_io, sprite_queue, bounds)
    }
}
