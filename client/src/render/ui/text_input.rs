use super::*;
use crate::bindable::SpriteColorMode;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use unicode_segmentation::UnicodeSegmentation;

pub struct TextInput {
    view_offset: Vec2,
    ime_area: Rect,
    ime_area_updated: bool,
    caret_index: usize,
    character_limit: usize,
    text: String,
    rendered_text: String,
    prev_pre_edit: Option<(String, Option<(usize, usize)>)>,
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
    pub fn new(game_io: &GameIO, font: FontName) -> Self {
        let globals = Globals::from_resources(game_io);

        Self {
            view_offset: Vec2::ZERO,
            ime_area: Rect::ZERO,
            ime_area_updated: false,
            caret_index: 0,
            character_limit: usize::MAX,
            text: String::new(),
            rendered_text: String::new(),
            prev_pre_edit: None,
            text_style: TextStyle::new(game_io, font),
            active: false,
            init_active: false,
            silent: false,
            caret_time: 0,
            solid_sprite: (globals.assets).new_sprite(game_io, ResourcePaths::PIXEL),
            paged: false,
            lines_per_page: 0,
            cached_metrics: TextMetrics {
                size: Vec2::ZERO,
                line_ranges: vec![0..0; 1],
            },
            sizing_dirty: false,
            filter_callback: Box::new(|_| true),
            change_callback: Box::new(|_| {}),
        }
    }

    pub fn with_str(mut self, text: &str) -> Self {
        self.text = String::from(text);
        self.rendered_text = String::from(text);
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

    /// Returns true if the text in prev_pre_edit is updated
    fn update_pre_edit(&mut self, input: &mut GameInputManager) -> bool {
        let pre_edit = input.text_pre_edit();

        // reset caret time for the cases where we exit early
        let mut prev_caret_time = self.caret_time;
        self.caret_time = 0;

        let Some((prev_text, selection)) = &mut self.prev_pre_edit else {
            self.prev_pre_edit = pre_edit.map(|(text, selection)| (text.to_string(), selection));

            let updated = self.prev_pre_edit.is_some();

            if !updated {
                self.caret_time = prev_caret_time;
            }

            return updated;
        };

        let Some((new_text, new_selection)) = pre_edit else {
            self.prev_pre_edit = None;
            return true;
        };

        if *selection != new_selection {
            *selection = new_selection;
            // prevent caret time from being restored, as we moved rendered caret position
            prev_caret_time = 0;
        }

        if prev_text != new_text {
            // reuse buffer
            prev_text.clear();
            prev_text.push_str(new_text);
            return true;
        }

        // restore caret time since nothing changed
        self.caret_time = prev_caret_time;

        false
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
        let input_util = InputUtil::new(game_io);

        if !self.active {
            if focused && (self.init_active || input_util.was_just_pressed(Input::Confirm)) {
                if !self.silent {
                    let globals = Globals::from_resources(game_io);
                    globals.audio.play_sound(&globals.sfx.cursor_select);
                }

                self.init_active = false;
                self.active = true;
                self.caret_time = 0;

                let input = game_io.input_mut();
                input.set_ime_cursor_area(self.ime_area);
                input.start_text_input();
            }

            return;
        }

        let controller_pressing_cancel = input_util.controller_just_pressed(Input::Cancel);

        if input_util.controller_just_pressed(Input::Confirm) {
            // open text input again, allows use to reopen input after accidental close
            let input = game_io.input_mut();

            input.set_ime_cursor_area(self.ime_area);
            input.start_text_input();
        }

        let input = game_io.input_mut();

        // test if we should confirm the input
        let holding_shift = input.is_key_down(Key::LShift) || input.is_key_down(Key::RShift);
        let pressed_return =
            input.was_key_just_pressed(Key::Return) || input.is_key_repeated(Key::Return);

        if input.was_key_just_pressed(Key::Escape)
            || controller_pressing_cancel
            || (!holding_shift && pressed_return)
            || (!self.paged && input.was_key_just_pressed(Key::Tab))
            || (cfg!(target_os = "android") && input.text().contains("\n"))
        {
            self.active = false;
            input.end_text_input();

            if !self.silent {
                let globals = Globals::from_resources(game_io);
                globals.audio.play_sound(&globals.sfx.cursor_move);
            }

            (self.change_callback)(&self.text);
            return;
        }

        // update ime area if it updated
        if input.accepting_text() && self.ime_area_updated {
            input.set_ime_cursor_area(self.ime_area);
            self.ime_area_updated = false;
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
                self.caret_index -= self.grapheme_before(self.caret_index).len();
            }
        }

        if input.was_key_just_pressed(Key::Right) || input.is_key_repeated(Key::Right) {
            if holding_ctrl {
                self.caret_index += forward_jump(&self.text, self.caret_index);
            } else if self.caret_index < self.text.len() {
                self.caret_index += self.grapheme_at(self.caret_index).len();
            }
        }

        // update pre edit
        let updated_pre_edit = self.update_pre_edit(input);

        // handle incoming text
        let mut incoming_text = input.text().to_string();

        if holding_ctrl && input.was_key_just_pressed(Key::V) {
            // pasting
            incoming_text.clone_from(&input.request_clipboard_text());
        }

        if updated_pre_edit || !incoming_text.is_empty() {
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
        let scissor_rect = bounds / RESOLUTION_F;
        sprite_queue.set_scissor(scissor_rect);

        // todo: if paged, use draw_sliced to render just the page
        let pre_edit_selection = self
            .prev_pre_edit
            .as_ref()
            .and_then(|(_, selection)| *selection)
            .map(|selection| {
                let start = selection.0.min(selection.1);
                let end = selection.0.max(selection.1);
                start + self.caret_index..end + self.caret_index
            })
            .unwrap_or_default();

        let pre_edit_range = self
            .prev_pre_edit
            .as_ref()
            .map(|(text, _)| self.caret_index..self.caret_index + text.len());

        let text_shadow_color = self.text_style.shadow_color;

        if let Some(pre_edit_range) = pre_edit_range {
            self.text_style.draw_slice_styled(
                game_io,
                sprite_queue,
                &self.rendered_text,
                0..self.rendered_text.len(),
                &mut move |sprite_queue, sprite, index| {
                    if index.is_none_or(|i| !pre_edit_range.contains(&i)) {
                        // draw plain
                        sprite_queue.draw_sprite(sprite);
                        return;
                    }

                    let prev_color = sprite.color();

                    if index.is_none_or(|i| !pre_edit_selection.contains(&i))
                        || sprite.color() == text_shadow_color
                    {
                        // render non selected pre edit text with transparency
                        sprite.set_color(prev_color.multiply_alpha(0.65));
                        sprite_queue.draw_sprite(sprite);
                        sprite.set_color(prev_color);
                        return;
                    }

                    let prev_color_mode = sprite_queue.color_mode();

                    // draw styled
                    sprite_queue.set_color_mode(SpriteColorMode::Multiply);
                    sprite.set_color(Color::GREEN);
                    sprite_queue.draw_sprite(sprite);

                    // reset styles
                    sprite_queue.set_color_mode(prev_color_mode);
                    sprite.set_color(prev_color);
                },
            );
        } else {
            self.text_style
                .draw(game_io, sprite_queue, &self.rendered_text);
        }

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

        // update ime area
        let mut ime_area = scissor_rect;
        ime_area.set_position(ime_area.position() * 2.0 - 1.0);
        ime_area.y *= -1.0;

        self.ime_area_updated |= self.ime_area != ime_area;
        self.ime_area = ime_area;
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
    fn rendered_caret_index(&self) -> usize {
        self.caret_index
            + self
                .prev_pre_edit
                .as_ref()
                .and_then(|(_, selection)| *selection)
                .map(|selection| selection.1)
                .unwrap_or_default()
    }

    fn caret_position(&self, bounds: Rect) -> Vec2 {
        let line_ranges = &self.cached_metrics.line_ranges;
        let caret_index = self.rendered_caret_index();

        let (line_index, range) = line_ranges
            .iter()
            .enumerate()
            .filter(|(_, range)| range.start <= caret_index)
            .next_back()
            .map(|(i, range)| (i, range.clone()))
            .unwrap_or_default();

        let before_caret_metrics = self
            .text_style
            .measure(&self.rendered_text[range.start..caret_index]);

        let caret_offset = Vec2::new(
            before_caret_metrics.size.x,
            line_index as f32 * self.text_style.line_height(),
        );

        bounds.position() + caret_offset
    }

    fn grapheme_at(&self, i: usize) -> &str {
        self.text[i..].graphemes(true).next().unwrap_or_default()
    }

    fn grapheme_before(&self, i: usize) -> &str {
        self.text[0..i]
            .graphemes(true)
            .next_back()
            .unwrap_or_default()
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
                        let char_len = self.grapheme_before(self.caret_index).len();

                        let range = self.caret_index - char_len..self.caret_index;
                        self.text.drain(range);
                        self.caret_index -= char_len;
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
                        let char_len = self.grapheme_at(self.caret_index).len();
                        let range = self.caret_index..self.caret_index + char_len;
                        self.text.drain(range);
                    }
                }

                _ => {
                    if !self.paged && grapheme == "\n" {
                        continue;
                    }

                    let is_valid_character = self.text_style.supports_character(grapheme);

                    if is_valid_character
                        && (self.character_limit == usize::MAX
                            || self.text.graphemes(true).count() < self.character_limit)
                    {
                        self.text.insert_str(self.caret_index, grapheme);
                        self.caret_index += grapheme.len();
                    }
                }
            }
        }

        // update rendered_text
        self.rendered_text.clear();
        self.rendered_text.push_str(&self.text[0..self.caret_index]);

        if let Some((text, _)) = &self.prev_pre_edit {
            self.rendered_text.push_str(text);
        }

        self.rendered_text.push_str(&self.text[self.caret_index..]);

        // resolve bounds
        let old_size = self.measure_ui_size(game_io);
        self.cached_metrics = self.text_style.measure(&self.rendered_text);

        if !self.paged {
            self.sizing_dirty = self.sizing_dirty || old_size != self.measure_ui_size(game_io);
        }
    }

    fn update_view_offset(&mut self, bounds: Rect) {
        let caret_position = self.caret_position(bounds);

        // invert for simpler logic, view offset is (-Infinity, 0.0] otherwise
        self.view_offset = -self.view_offset;

        if self.paged {
            // vertical scroll
            let page_height = self.text_style.line_height() * self.lines_per_page as f32;
            self.view_offset.y = (caret_position.y / page_height).floor() * page_height;
        } else {
            // horizontal scroll
            self.view_offset.x = self.view_offset.x.min(caret_position.x);

            // + 1.0 for caret width
            if self.view_offset.x + bounds.width < caret_position.x + 1.0 {
                self.view_offset.x = caret_position.x - bounds.width + 1.0;
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
        .next_back()
        .map(|s| s.len())
        .unwrap_or_default()
}
