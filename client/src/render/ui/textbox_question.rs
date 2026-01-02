use super::{TextStyle, TextboxCursor, TextboxInterface, UiInputTracker};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

struct RenderData {
    yes_position: Vec2,
    no_position: Vec2,
    cursor: TextboxCursor,
}

pub struct TextboxQuestion {
    message: String,
    response_start: usize,
    yes_string: String,
    no_string: String,
    selection: bool,
    complete: bool,
    callback: Option<Box<dyn FnOnce(bool)>>,
    render_data: Option<RenderData>,
    input_tracker: UiInputTracker,
}

impl TextboxQuestion {
    pub fn new(game_io: &GameIO, message: String, callback: impl FnOnce(bool) + 'static) -> Self {
        let globals = Globals::from_resources(game_io);

        let response_start = message.len() + 1;
        let yes_string = globals.translate("navigation-yes");
        let no_string = globals.translate("navigation-no");

        Self {
            message: format!("{message}\n\x02       {yes_string:<7} {no_string}",),
            response_start,
            yes_string,
            no_string,
            complete: false,
            selection: true,
            callback: Some(Box::new(callback)),
            render_data: None,
            input_tracker: UiInputTracker::new(),
        }
    }
}

impl TextboxInterface for TextboxQuestion {
    fn text(&self) -> &str {
        &self.message
    }

    fn is_complete(&self) -> bool {
        self.complete
    }

    fn update(&mut self, game_io: &mut GameIO, text_style: &TextStyle, lines: usize) {
        if self.complete {
            return;
        }

        if self.render_data.is_none() {
            // initialize
            let line_height = text_style.line_height();
            let text_height = line_height * lines as f32;
            let left = Vec2::new(
                text_style.bounds.x,
                text_style.bounds.y + text_height - line_height * 0.5,
            );

            let last_line = &self.message[self.response_start..];

            let yes_index = last_line.find(&self.yes_string).unwrap();
            let yes_offset = text_style.measure(&last_line[..yes_index]).size.x;
            let no_index = last_line.rfind(&self.no_string).unwrap();
            let no_offset = text_style.measure(&last_line[..no_index]).size.x;

            let render_data = RenderData {
                yes_position: left + Vec2::new(yes_offset, 0.0),
                no_position: left + Vec2::new(no_offset, 0.0),
                cursor: TextboxCursor::new(game_io),
            };
            self.render_data = Some(render_data);
        }

        if game_io.is_in_transition() {
            return;
        }

        let input_util = InputUtil::new(game_io);
        self.input_tracker.update(game_io);

        if self.input_tracker.pulsed(Input::Left) || self.input_tracker.pulsed(Input::Right) {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);

            self.selection = !self.selection;
        }

        if input_util.was_just_pressed(Input::Confirm) {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);

            self.complete = true;
        }

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            self.selection = false;
            self.complete = true;
        }

        if self.complete {
            let callback = self.callback.take().unwrap();
            callback(self.selection);
        }
    }

    fn draw(&mut self, _game_io: &framework::prelude::GameIO, sprite_queue: &mut SpriteColorQueue) {
        let Some(render_data) = &mut self.render_data else {
            return;
        };

        let position = if self.selection {
            render_data.yes_position
        } else {
            render_data.no_position
        };

        render_data.cursor.set_position(position);
        render_data.cursor.update();
        render_data.cursor.draw(sprite_queue);
    }
}
