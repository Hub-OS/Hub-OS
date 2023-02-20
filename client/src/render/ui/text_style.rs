use super::FontStyle;
use crate::bindable::SpriteColorMode;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::ops::Range;
use std::sync::Arc;
use unicode_segmentation::UnicodeSegmentation;

pub struct TextMetrics {
    pub line_ranges: Vec<Range<usize>>,
    pub size: Vec2,
}

impl TextMetrics {
    pub fn line_breaks(&self) -> impl Iterator<Item = usize> + '_ {
        self.line_ranges[1..].iter().map(|range| range.start)
    }
}

#[derive(Clone)]
pub struct TextStyle {
    pub font_animator: Arc<Animator>,
    pub font_style: FontStyle,
    pub letter_spacing: f32,
    pub line_spacing: f32,
    pub scale: Vec2,
    pub color: Color,
    pub shadow_color: Color,
    /// Used for handling text wrapping
    pub bounds: Rect,
    /// Every letter will be treated as having the same size as the letter A during text placement
    pub monospace: bool,
}

impl TextStyle {
    pub fn new(game_io: &GameIO, font_style: FontStyle) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();

        Self {
            font_animator: globals.font_animator.clone(),
            font_style,
            letter_spacing: 1.0,
            line_spacing: 1.0,
            scale: Vec2::ONE,
            color: Color::WHITE,
            shadow_color: Color::TRANSPARENT,
            bounds: Rect::new(0.0, 0.0, std::f32::INFINITY, std::f32::INFINITY),
            monospace: false,
        }
    }

    pub fn new_monospace(game_io: &GameIO, font_style: FontStyle) -> Self {
        let mut style = Self::new(game_io, font_style);
        style.monospace = true;

        style
    }

    pub fn with_bounds(mut self, bounds: Rect) -> Self {
        self.bounds = bounds;
        self
    }

    pub fn with_color(mut self, color: Color) -> Self {
        self.color = color;
        self
    }

    pub fn with_shadow_color(mut self, color: Color) -> Self {
        self.shadow_color = color;
        self
    }

    pub fn with_line_spacing(mut self, line_spacing: f32) -> Self {
        self.line_spacing = line_spacing;
        self
    }

    pub fn line_height(&self) -> f32 {
        let frame = self.character_frame("A");
        frame.size().y * self.scale.y + self.line_spacing
    }

    pub fn measure(&self, text: &str) -> TextMetrics {
        self.iterate(text, |_, _| {})
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, text: &str) {
        self.draw_slice(game_io, sprite_queue, text, 0..text.len());
    }

    pub fn draw_slice(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        text: &str,
        range: Range<usize>,
    ) {
        let prev_color_mode = sprite_queue.color_mode();

        let globals = game_io.resource::<Globals>().unwrap();
        let mut sprite = Sprite::new(game_io, globals.font_texture.clone());

        sprite.set_scale(self.scale);

        // draw shadow
        if self.shadow_color != Color::TRANSPARENT {
            Self::update_sprite_color(&mut sprite, sprite_queue, self.shadow_color);
            self.iterate_slice(text, range.clone(), |frame, position| {
                frame.apply(&mut sprite);

                sprite.set_position(position + Vec2::ONE);
                sprite_queue.draw_sprite(&sprite);
            });
        }

        // draw normal
        Self::update_sprite_color(&mut sprite, sprite_queue, self.color);
        self.iterate_slice(text, range, |frame, position| {
            frame.apply(&mut sprite);

            sprite.set_position(position);
            sprite_queue.draw_sprite(&sprite);
        });

        sprite_queue.set_color_mode(prev_color_mode);
    }

    fn update_sprite_color(sprite: &mut Sprite, sprite_queue: &mut SpriteColorQueue, color: Color) {
        if color == Color::WHITE {
            // avoid switching pipelines / updating uniforms if we can
            sprite.set_color(sprite_queue.color_mode().default_color());
        } else {
            sprite.set_color(color);
            sprite_queue.set_color_mode(SpriteColorMode::Multiply);
        }
    }

    fn iterate<F>(&self, text: &str, callback: F) -> TextMetrics
    where
        F: FnMut(AnimationFrame, Vec2),
    {
        self.iterate_slice(text, 0..text.len(), callback)
    }

    fn iterate_slice<F>(&self, text: &str, range: Range<usize>, mut callback: F) -> TextMetrics
    where
        F: FnMut(AnimationFrame, Vec2),
    {
        let mut insert_tracker = TextInsertTracker::new(self);
        insert_tracker.line_start_index = range.start;

        let mut max_x: f32 = 0.0;
        let mut max_y: f32 = 0.0;
        let mut last_index = 0;

        'primary: for (word_index, word) in word_indices(text) {
            if word_index + word.len() - 1 < range.start {
                continue;
            }

            let word_width = self.measure_word(word);

            if (insert_tracker.x + word_width) * self.scale.x > self.bounds.width {
                if word == " " || word == "\t" {
                    // create a new line and eat the whitespace
                    insert_tracker.new_line(word_index, 1);
                    continue;
                }

                if insert_tracker.line_start_index != word_index {
                    // word too long, move it to a new line if it's not already on one
                    insert_tracker.new_line(word_index, 0);
                }
            }

            for (relative_index, character) in word.grapheme_indices(true) {
                let index = word_index + relative_index;

                if index < range.start {
                    continue;
                }

                if index >= range.end {
                    break;
                }

                match character {
                    " " => {
                        insert_tracker.insert_space(index);
                    }
                    "\t" => {
                        insert_tracker.insert_tab(index);
                    }
                    "\r\n" | "\n" => {
                        insert_tracker.new_line(index, 1);
                    }
                    _ => {
                        let frame = self.character_frame(character);

                        if !frame.valid {
                            continue;
                        }

                        let character_size = frame.size();
                        let (x, y) = insert_tracker.next_position(index, character_size).into();

                        // break early if we can't place this character
                        let line_bottom = insert_tracker.y + insert_tracker.whitespace.y;

                        if line_bottom > self.bounds.height / self.scale.y {
                            break 'primary;
                        }

                        let position = self.bounds.position() + Vec2::new(x, y) * self.scale;

                        callback(frame, position);

                        max_y = max_y.max(y + character_size.y);
                    }
                }

                // break early if whitespace caused us to go out of bounds
                let line_bottom = insert_tracker.y + insert_tracker.whitespace.y;

                if line_bottom > self.bounds.height / self.scale.y {
                    break 'primary;
                }

                max_x = max_x.max(insert_tracker.x);
                max_y = line_bottom;
                last_index = index;
            }
        }

        if max_x > 0.0 {
            // trim off extra spacing
            max_x -= self.letter_spacing;
        }

        let range_start = insert_tracker.line_start_index;
        let range_end = text.len().min(last_index + 1);
        insert_tracker.line_ranges.push(range_start..range_end);

        TextMetrics {
            line_ranges: insert_tracker.line_ranges,
            size: Vec2::new(max_x * self.scale.x, max_y * self.scale.y),
        }
    }

    fn measure_word(&self, word: &str) -> f32 {
        match word {
            " " => self.character_frame("A").size().x,
            "\t" => self.character_frame("A").size().x * 4.0,
            "" | "\n" => 0.0,
            _ => {
                let mut width = 0.0;

                for character in word.graphemes(true) {
                    let frame = self.character_frame(character);
                    width += frame.size().x + self.letter_spacing;
                }

                width - self.letter_spacing
            }
        }
    }

    pub fn supports_character(&self, character: &str) -> bool {
        let state = self.character_state(character);

        self.font_animator
            .frame_list(&state)
            .and_then(|frame_list| frame_list.frame(0))
            .is_some()
            || character == " "
            || character == "\t"
            || character == "\n"
    }

    fn character_frame(&self, character: &str) -> AnimationFrame {
        let state = self.character_state(character);

        self.font_animator
            .frame_list(&state)
            .and_then(|frame_list| frame_list.frame(0).cloned())
            .unwrap_or_default()
    }

    fn character_state(&self, character: &str) -> String {
        use core::fmt::Write;

        let state_prefix = self.font_style.state_prefix();

        let mut state = String::with_capacity(state_prefix.len() + 6);

        // begin with the state prefix
        let _ = write!(state, "{state_prefix}");

        // convert the character string to 6 hex chars
        for c in character.chars() {
            let _ = write!(state, "{:06x}", c as u32);
        }

        state
    }
}

struct TextInsertTracker<'a> {
    x: f32,
    y: f32,
    whitespace: Vec2,
    style: &'a TextStyle,
    line_start_index: usize,
    line_ranges: Vec<Range<usize>>,
}

impl<'a> TextInsertTracker<'a> {
    fn new(style: &'a TextStyle) -> Self {
        Self {
            x: 0.0,
            y: 0.0,
            whitespace: style.character_frame("A").size(),
            style,
            line_start_index: 0,
            line_ranges: Vec::new(),
        }
    }

    fn next_position(&mut self, index: usize, character_size: Vec2) -> Vec2 {
        let width_used = if self.style.monospace {
            self.whitespace.x
        } else {
            character_size.x
        };

        if self.x + width_used > self.style.bounds.width / self.style.scale.x {
            // wrap text
            self.new_line(index, 0);
        }

        // center the character for monospace
        let placement_x = self.x + ((width_used - character_size.x) * 0.5).floor();

        self.x += width_used + self.style.letter_spacing;

        let offset_y = (self.whitespace.y - character_size.y).max(0.0);

        Vec2::new(placement_x, self.y + offset_y)
    }

    fn insert_space(&mut self, index: usize) {
        self.next_position(index, self.whitespace);
    }

    fn insert_tab(&mut self, index: usize) {
        self.next_position(index, Vec2::new(self.whitespace.x * 4.0, 0.0));
    }

    fn new_line(&mut self, index: usize, byte_length: usize) {
        self.y += self.whitespace.y + self.style.line_spacing;
        self.x = 0.0;

        self.line_ranges.push(self.line_start_index..index);
        self.line_start_index = index + byte_length;
    }
}

fn word_indices(text: &str) -> impl Iterator<Item = (usize, &str)> {
    use itertools::Itertools;

    text.grapheme_indices(true).peekable().batching(|it| {
        let mut count = 0;
        let mut space_found = false;

        let mut word_it = it.peeking_take_while(move |(_, grapheme)| {
            count += 1;

            match *grapheme {
                "\n" | "\t" | " " => {
                    space_found = true;

                    count == 1
                }
                _ => !space_found,
            }
        });

        let (first_index, grapheme) = word_it.next()?;
        let (last_index, grapheme) = word_it.last().unwrap_or((first_index, grapheme));

        Some((first_index, &text[first_index..last_index + grapheme.len()]))
    })
}
