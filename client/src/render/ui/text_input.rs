use super::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use unicode_segmentation::UnicodeSegmentation;

pub struct TextInput {
    view_offset: Vec2,
    caret_index: usize,
    character_limit: usize,
    text: String,
    text_style: TextStyle,
    active: bool,
    init_active: bool,
    silent: bool,
    caret_time: FrameTime,
    solid_sprite: Sprite,
    paged: bool,
    lines_per_page: usize,
    cached_metrics: TextMetrics,
    sizing_dirty: bool,
    filter_callback: Box<dyn Fn(&str) -> bool>,
    change_callback: Box<dyn Fn(&str)>,
}

impl TextInput {
    pub fn new(game_io: &GameIO, font_style: FontStyle) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();

        Self {
            view_offset: Vec2::ZERO,
            caret_index: 0,
            character_limit: usize::MAX,
            text: String::new(),
            text_style: TextStyle::new(game_io, font_style),
            active: false,
            init_active: false,
            silent: false,
            caret_time: 0,
            solid_sprite: (globals.assets).new_sprite(game_io, ResourcePaths::WHITE_PIXEL),
            paged: false,
            lines_per_page: 0,
            cached_metrics: TextMetrics {
                size: Vec2::ZERO,
                line_ranges: vec![0..0],
            },
            sizing_dirty: false,
            filter_callback: Box::new(|_| true),
            change_callback: Box::new(|_| {}),
        }
    }

    pub fn with_str(mut self, text: &str) -> Self {
        self.text = String::from(text);
        self.caret_index = text.len();
        self.cached_metrics = self.text_style.measure(text);
        self
    }

    pub fn with_color(mut self, color: Color) -> Self {
        self.text_style.color = color;
        self
    }

    pub fn with_shadow_color(mut self, color: Color) -> Self {
        self.text_style.shadow_color = color;
        self
    }

    pub fn with_character_limit(mut self, limit: usize) -> Self {
        self.character_limit = limit;
        self
    }

    pub fn with_silent(mut self, silent: bool) -> Self {
        self.silent = silent;
        self
    }

    pub fn with_active(mut self, active: bool) -> Self {
        self.init_active = active;
        self
    }

    pub fn with_filter(mut self, callback: impl Fn(&str) -> bool + 'static) -> Self {
        self.filter_callback = Box::new(callback);
        self
    }

    pub fn on_change(mut self, callback: impl Fn(&str) + 'static) -> Self {
        self.change_callback = Box::new(callback);
        self
    }

    pub fn with_paged(mut self, page_size: Vec2) -> Self {
        self.paged = true;
        self.lines_per_page = (page_size.y / self.text_style.line_height()) as usize;
        self.text_style.bounds.width = page_size.x;
        self
    }
}

impl UiNode for TextInput {
    fn focusable(&self) -> bool {
        true
    }

    fn is_locking_focus(&self) -> bool {
        self.active
    }

    fn update(&mut self, game_io: &mut GameIO, bounds: Rect, focused: bool) {
        if !self.active {
            let input_util = InputUtil::new(game_io);

            if focused && (self.init_active || input_util.was_just_pressed(Input::Confirm)) {
                if !self.silent {
                    let globals = game_io.resource::<Globals>().unwrap();
                    globals.audio.play_sound(&globals.sfx.cursor_select);
                }

                self.init_active = false;
                self.active = true;
                self.caret_time = 0;
                game_io.input_mut().start_text_input();
            }

            return;
        }

        let input = game_io.input_mut();
        let holding_shift = input.is_key_down(Key::LShift) || input.is_key_down(Key::RShift);
        let pressed_return =
            input.was_key_just_pressed(Key::Return) || input.is_key_repeated(Key::Return);

        if input.was_key_just_pressed(Key::Escape)
            || (!holding_shift && pressed_return)
            || (!self.paged && input.was_key_just_pressed(Key::Tab))
        {
            self.active = false;
            input.end_text_input();

            if !self.silent {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.sfx.cursor_move);
            }

            (self.change_callback)(&self.text);
            return;
        }

        // update caret
        self.caret_time += 1;

        let old_caret_index = self.caret_index;
        let holding_ctrl = input.is_key_down(Key::LControl) || input.is_key_down(Key::RControl);

        if input.was_key_just_pressed(Key::Home) || input.is_key_repeated(Key::Home) {
            self.caret_index = 0;
        }

        if input.was_key_just_pressed(Key::End) || input.is_key_repeated(Key::End) {
            self.caret_index = self.text.len();
        }

        if input.was_key_just_pressed(Key::Left) || input.is_key_repeated(Key::Left) {
            if holding_ctrl {
                self.caret_index -= backward_jump(&self.text, self.caret_index);
            } else if self.caret_index > 0 {
                self.caret_index -= 1;
            }
        }

        if input.was_key_just_pressed(Key::Right) || input.is_key_repeated(Key::Right) {
            if holding_ctrl {
                self.caret_index += forward_jump(&self.text, self.caret_index);
            } else if self.caret_index < self.text.len() {
                self.caret_index += 1;
            }
        }

        let mut incoming_text = input.text().to_string();

        if holding_ctrl && input.was_key_just_pressed(Key::V) {
            // pasting
            incoming_text += input.request_clipboard_text().as_str();
        }

        if !incoming_text.is_empty() {
            // update text
            self.insert_text(game_io, &incoming_text, holding_ctrl);
        }

        if self.caret_index != old_caret_index {
            self.caret_time = 0;
        }

        self.update_view_offset(bounds);
    }

    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
        (self.text_style.bounds).set_position(bounds.position() + self.view_offset);

        // restrict rendering to bounds
        sprite_queue.set_scissor(bounds / RESOLUTION_F);

        // todo: if paged, use draw_sliced to render just the page
        self.text_style.draw(game_io, sprite_queue, &self.text);

        if self.active && self.caret_time % 60 < 30 {
            // draw cursor
            let caret_position = self.caret_position(bounds) + self.view_offset;
            self.solid_sprite.set_position(caret_position);

            let line_height = self.text_style.line_height();
            self.solid_sprite.set_height(line_height);

            self.solid_sprite.set_color(self.text_style.color);
            sprite_queue.draw_sprite(&self.solid_sprite);
        }

        // reset scissor
        sprite_queue.set_scissor(Rect::UNIT);
    }

    fn ui_size_dirty(&self) -> bool {
        self.sizing_dirty
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        if self.paged {
            Vec2::new(
                self.text_style.bounds.width,
                self.text_style.line_height() * self.lines_per_page as f32,
            )
        } else {
            self.cached_metrics.size
        }
    }
}

impl TextInput {
    fn caret_position(&self, bounds: Rect) -> Vec2 {
        let line_ranges = &self.cached_metrics.line_ranges;

        let (line_index, range) = line_ranges
            .iter()
            .enumerate()
            .filter(|(_, range)| range.start <= self.caret_index)
            .last()
            .map(|(i, range)| (i, range.clone()))
            .unwrap();

        let before_caret_metrics = self
            .text_style
            .measure(&self.text[range.start..self.caret_index]);

        let caret_offset = Vec2::new(
            before_caret_metrics.size.x,
            line_index as f32 * self.text_style.line_height() + 1.0,
        );

        bounds.position() + caret_offset
    }

    fn insert_text(&mut self, game_io: &GameIO, text: &str, holding_ctrl: bool) {
        for grapheme in text.graphemes(true) {
            if !(self.filter_callback)(grapheme) {
                continue;
            }

            match grapheme {
                // BACKSPACE
                "\u{8}" => {
                    if holding_ctrl {
                        // remove until the previous boundary
                        let end = self.caret_index;
                        let len = backward_jump(&self.text, end);

                        let start = end - len;
                        self.text.replace_range(start..end, "");
                        self.caret_index -= len;
                    } else if self.caret_index > 0 {
                        // remove one character
                        self.text.remove(self.caret_index - 1);
                        self.caret_index -= 1;
                    }
                }

                // DELETE
                "\u{7f}" => {
                    if holding_ctrl {
                        // remove until the next boundary
                        let start = self.caret_index;
                        let len = forward_jump(&self.text, start);

                        let end = start + len;
                        self.text.replace_range(start..end, "");
                    } else if self.caret_index < self.text.len() {
                        // remove one character
                        self.text.remove(self.caret_index);
                    }
                }

                _ => {
                    if !self.paged && grapheme == "\n" {
                        continue;
                    }

                    let is_valid_character = self.text_style.supports_character(grapheme);

                    if is_valid_character
                        && self.text.len() + grapheme.len() <= self.character_limit
                    {
                        self.text.insert_str(self.caret_index, grapheme);
                        self.caret_index += grapheme.len();
                    }
                }
            }
        }

        let old_size = self.measure_ui_size(game_io);
        self.cached_metrics = self.text_style.measure(&self.text);

        if !self.paged {
            self.sizing_dirty = self.sizing_dirty || old_size != self.measure_ui_size(game_io);
        }
    }

    fn update_view_offset(&mut self, bounds: Rect) {
        // measure
        let metrics = self.text_style.measure(&self.text[..self.caret_index]);
        let caret_offset = metrics.size;

        // invert for simpler logic, view offset is (-Infinity, 0.0] otherwise
        self.view_offset = -self.view_offset;

        if self.paged {
            // vertical scroll
            let page_height = self.text_style.line_height() * self.lines_per_page as f32;
            self.view_offset.y = (caret_offset.y / page_height).floor() * page_height;
        } else {
            // horizontal scroll
            self.view_offset.x = self.view_offset.x.min(caret_offset.x);

            // + 1.0 for caret width
            if self.view_offset.x + bounds.width < caret_offset.x + 1.0 {
                self.view_offset.x = caret_offset.x - bounds.width + 1.0;
            }
        }

        // revert
        self.view_offset = -self.view_offset;
    }
}

fn forward_jump(text: &str, start: usize) -> usize {
    text[start..]
        .split_word_bounds()
        .next()
        .map(|s| s.len())
        .unwrap_or_default()
}

fn backward_jump(text: &str, end: usize) -> usize {
    text[..end]
        .split_word_bounds()
        .rev()
        .next()
        .map(|s| s.len())
        .unwrap_or_default()
}
