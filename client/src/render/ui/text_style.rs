use super::FontName;
use super::GlyphAtlas;
use crate::bindable::SpriteColorMode;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use packets::structures::TextStyleBlueprint;
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
    pub glyph_atlas: Arc<GlyphAtlas>,
    pub font: FontName,
    pub min_glyph_width: f32,
    pub letter_spacing: f32,
    pub line_spacing: f32,
    pub scale: Vec2,
    pub color: Color,
    pub shadow_color: Color,
    /// Used for handling text wrapping
    pub bounds: Rect,
    /// Every letter will be treated as having the same size as the letter A during text placement
    pub monospace: bool,
    pub ellipsis: bool,
}

impl TextStyle {
    pub fn new(game_io: &GameIO, font: FontName) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();

        Self::new_with_atlas(globals.glyph_atlas.clone(), font)
    }

    pub fn new_with_atlas(glyph_atlas: Arc<GlyphAtlas>, font: FontName) -> Self {
        Self {
            glyph_atlas,
            font,
            min_glyph_width: 0.0,
            letter_spacing: 1.0,
            line_spacing: 1.0,
            scale: Vec2::ONE,
            color: Color::WHITE,
            shadow_color: Color::TRANSPARENT,
            bounds: Rect::new(0.0, 0.0, f32::INFINITY, f32::INFINITY),
            monospace: false,
            ellipsis: false,
        }
    }

    pub fn new_monospace(game_io: &GameIO, font: FontName) -> Self {
        let mut style = Self::new(game_io, font);
        style.monospace = true;

        style
    }

    pub fn from_blueprint(
        game_io: &GameIO,
        assets: &impl AssetManager,
        blueprint: TextStyleBlueprint,
    ) -> Self {
        let glyph_atlas = if let Some(paths) = blueprint.custom_atlas {
            assets.glyph_atlas(game_io, &paths.texture, &paths.animation)
        } else {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.glyph_atlas.clone()
        };

        Self {
            glyph_atlas,
            font: if blueprint.font_name.is_empty() {
                FontName::Thin
            } else {
                FontName::from_name(&blueprint.font_name)
            },
            min_glyph_width: blueprint.min_glyph_width,
            letter_spacing: blueprint.letter_spacing,
            line_spacing: blueprint.line_spacing,
            scale: Vec2::new(blueprint.scale_x, blueprint.scale_y),
            color: blueprint.color.into(),
            shadow_color: blueprint.shadow_color.into(),
            bounds: Rect::new(0.0, 0.0, f32::INFINITY, f32::INFINITY), // todo: ? not used currently
            monospace: blueprint.monospace,
            ellipsis: false,
        }
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

    pub fn with_min_glyph_width(mut self, width: f32) -> Self {
        self.min_glyph_width = width;
        self
    }

    pub fn with_line_spacing(mut self, line_spacing: f32) -> Self {
        self.line_spacing = line_spacing;
        self
    }

    pub fn with_ellipsis(mut self, ellipsis: bool) -> Self {
        self.ellipsis = ellipsis;
        self
    }

    pub fn line_height(&self) -> f32 {
        let whitespace_size = self.glyph_atlas.resolve_whitespace_size(&self.font);
        (whitespace_size.y + self.line_spacing) * self.scale.y
    }

    pub fn measure(&self, text: &str) -> TextMetrics {
        self.iterate(text, |_, _, _| {})
    }

    pub fn measure_grapheme(&self, grapheme: &str) -> Vec2 {
        let mut size = self.glyph_atlas.resolve_whitespace_size(&self.font);

        if !self.monospace && grapheme != " " {
            let frame = self.character_frame(grapheme);
            size.x = frame.size().x - frame.origin.x;
        }

        size * self.scale
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, text: &str) {
        self.draw_styled(
            game_io,
            sprite_queue,
            text,
            &mut |sprite_queue, sprite, _| sprite_queue.draw_sprite(sprite),
        );
    }

    pub fn draw_styled(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        text: &str,
        mut draw_styled_fn: &mut dyn FnMut(&mut SpriteColorQueue, &mut Sprite, Option<usize>),
    ) {
        let prev_color_mode = sprite_queue.color_mode();
        let draw_fn = &mut draw_styled_fn;

        let mut sprite = Sprite::new(game_io, self.glyph_atlas.texture().clone());

        sprite.set_scale(self.scale);

        // draw shadow
        if self.shadow_color != Color::TRANSPARENT {
            Self::update_sprite_color(&mut sprite, sprite_queue, self.shadow_color);
            self.draw_single_run(
                sprite_queue,
                &mut sprite,
                text,
                &mut |sprite_queue, sprite, index| {
                    sprite.set_position(sprite.position() + self.scale);
                    draw_fn(sprite_queue, sprite, index);
                },
            );
        }

        // normal run
        Self::update_sprite_color(&mut sprite, sprite_queue, self.color);
        self.draw_single_run(sprite_queue, &mut sprite, text, draw_fn);

        sprite_queue.set_color_mode(prev_color_mode);
    }

    fn draw_single_run(
        &self,
        sprite_queue: &mut SpriteColorQueue,
        sprite: &mut Sprite,
        text: &str,
        draw_fn: &mut dyn FnMut(&mut SpriteColorQueue, &mut Sprite, Option<usize>),
    ) {
        self.iterate(text, move |frame, position, index| {
            frame.apply(sprite);
            sprite.set_position(position);

            draw_fn(sprite_queue, sprite, index);
        });
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

    pub fn iterate<F>(&self, text: &str, mut callback: F) -> TextMetrics
    where
        F: FnMut(AnimationFrame, Vec2, Option<usize>),
    {
        self.iterate_impl(text, &mut callback)
    }

    fn iterate_impl(
        &self,
        text: &str,
        mut callback: &mut dyn FnMut(AnimationFrame, Vec2, Option<usize>),
    ) -> TextMetrics {
        let mut insert_tracker = TextInsertTracker::new(self);
        insert_tracker.line_start_index = 0;

        let mut max_x: f32 = 0.0;
        let mut max_y: f32 = 0.0;
        let mut last_index = 0;
        let mut last_grapheme = "";

        'primary: for (word_index, word) in word_indices(text) {
            let word_width = self.measure_unbroken(word);
            let on_last_line = insert_tracker.y + insert_tracker.whitespace.y * 2.0
                > insert_tracker.unscaled_bounds_size.y;

            if (insert_tracker.x + word_width) * self.scale.x > self.bounds.width {
                if word == " " || word == "\t" {
                    if self.ellipsis && on_last_line {
                        self.output_ellipsis(&mut insert_tracker, &mut callback);
                        max_x = max_x.max(insert_tracker.x);
                        break 'primary;
                    }

                    // create a new line and eat the whitespace
                    insert_tracker.new_line(word_index, 1);
                    continue;
                }

                if !(self.ellipsis && on_last_line) && insert_tracker.line_start_index != word_index
                {
                    // word too long, move it to a new line if it's not already on one
                    insert_tracker.new_line(word_index, 0);
                }
            }

            let may_ellipse =
                self.ellipsis && on_last_line && insert_tracker.should_ellipse_before(word_width);

            for (relative_index, character) in word.grapheme_indices(true) {
                let index = word_index + relative_index;

                match character {
                    " " => {
                        let whitespace_x = insert_tracker.whitespace.x;

                        if may_ellipse && insert_tracker.should_ellipse_before(whitespace_x) {
                            self.output_ellipsis(&mut insert_tracker, &mut callback);
                            max_x = max_x.max(insert_tracker.x);
                            break 'primary;
                        }

                        insert_tracker.insert_space(index);
                    }
                    "\t" => {
                        let whitespace_x = insert_tracker.whitespace.x;

                        if may_ellipse && insert_tracker.should_ellipse_before(whitespace_x * 4.0) {
                            self.output_ellipsis(&mut insert_tracker, &mut callback);
                            max_x = max_x.max(insert_tracker.x);
                            break 'primary;
                        }

                        insert_tracker.insert_tab(index);
                    }
                    "\r\n" => {
                        if self.ellipsis && on_last_line {
                            self.output_ellipsis(&mut insert_tracker, &mut callback);
                            max_x = max_x.max(insert_tracker.x);
                            break 'primary;
                        }

                        insert_tracker.new_line(index, 2);
                    }
                    "\n" => {
                        if self.ellipsis && on_last_line {
                            self.output_ellipsis(&mut insert_tracker, &mut callback);
                            max_x = max_x.max(insert_tracker.x);
                            break 'primary;
                        }

                        insert_tracker.new_line(index, 1);
                    }
                    _ => {
                        let frame = self.character_frame(character);

                        if !frame.valid {
                            continue;
                        }

                        let prev_x = insert_tracker.x;
                        let character_size = frame.size() - frame.origin;
                        let (x, y) = insert_tracker.next_position(index, character_size).into();

                        if may_ellipse && insert_tracker.should_ellipse_before(0.0) {
                            insert_tracker.x = prev_x;
                            self.output_ellipsis(&mut insert_tracker, &mut callback);
                            max_x = max_x.max(insert_tracker.x);
                            break 'primary;
                        }

                        // break early if we can't place this character
                        let line_bottom = insert_tracker.y + insert_tracker.whitespace.y;

                        if line_bottom > insert_tracker.unscaled_bounds_size.y {
                            break 'primary;
                        }

                        let position = self.bounds.position() + Vec2::new(x, y) * self.scale;

                        callback(frame, position, Some(index));

                        max_y = max_y.max(y + character_size.y);
                    }
                }

                // break early if whitespace caused us to go out of bounds
                let line_bottom = insert_tracker.y + insert_tracker.whitespace.y;

                if line_bottom > insert_tracker.unscaled_bounds_size.y {
                    break 'primary;
                }

                max_x = max_x.max(insert_tracker.x);
                max_y = line_bottom;
                last_index = index;
                last_grapheme = character;
            }
        }

        if max_x > 0.0 {
            // trim off extra spacing
            max_x -= self.letter_spacing;
        }

        let range_start = insert_tracker.line_start_index;
        let range_end = text.len().min(last_index + last_grapheme.len());

        if range_start < range_end {
            insert_tracker.line_ranges.push(range_start..range_end);
        }

        TextMetrics {
            line_ranges: insert_tracker.line_ranges,
            size: Vec2::new(max_x * self.scale.x, max_y * self.scale.y),
        }
    }

    fn output_ellipsis(
        &self,
        insert_tracker: &mut TextInsertTracker,
        callback: &mut dyn FnMut(AnimationFrame, Vec2, Option<usize>),
    ) {
        let frame = self.character_frame(".");

        if !frame.valid {
            return;
        }

        let mut ellipsis_tracker = TextInsertTracker::new(self);
        let start_position = Vec2::new(insert_tracker.x + self.letter_spacing, insert_tracker.y);

        for _ in 0..3 {
            let offset = ellipsis_tracker.next_position(0, frame.size());

            let position = self.bounds.position() + (start_position + offset) * self.scale;
            callback(frame.clone(), position, None);
        }

        insert_tracker.x += ellipsis_tracker.x;
    }

    fn measure_unbroken(&self, word: &str) -> f32 {
        let whitespace_size = self.glyph_atlas.resolve_whitespace_size(&self.font);

        match word {
            " " => whitespace_size.x,
            "\t" => whitespace_size.x * 4.0,
            "" | "\n" | "\r\n" => 0.0,
            _ => {
                let mut width = 0.0;

                if self.monospace {
                    let total_chars = word.graphemes(true).count() as f32;
                    width += total_chars * (whitespace_size.x + self.letter_spacing);
                } else {
                    for character in word.graphemes(true) {
                        let frame = self.character_frame(character);
                        let glyph_width = frame.size().x - frame.origin.x;
                        width += glyph_width.max(self.min_glyph_width) + self.letter_spacing;
                    }
                }

                width - self.letter_spacing
            }
        }
    }

    pub fn supports_character(&self, character: &str) -> bool {
        self.glyph_atlas
            .character_frame(&self.font, character)
            .is_some()
            || character == " "
            || character == "\t"
            || character == "\n"
    }

    fn character_frame(&self, character: &str) -> AnimationFrame {
        self.glyph_atlas
            .character_frame(&self.font, character)
            .cloned()
            .unwrap_or_default()
    }
}

struct TextInsertTracker<'a> {
    x: f32,
    y: f32,
    unscaled_bounds_size: Vec2,
    whitespace: Vec2,
    ellipsis_width: f32,
    style: &'a TextStyle,
    line_start_index: usize,
    line_ranges: Vec<Range<usize>>,
}

impl<'a> TextInsertTracker<'a> {
    fn new(style: &'a TextStyle) -> Self {
        Self {
            x: 0.0,
            y: 0.0,
            unscaled_bounds_size: style.bounds.size() / style.scale,
            whitespace: style.glyph_atlas.resolve_whitespace_size(&style.font),
            ellipsis_width: if style.ellipsis {
                style.measure_unbroken("...") + style.letter_spacing
            } else {
                Default::default()
            },
            style,
            line_start_index: 0,
            line_ranges: Vec::new(),
        }
    }

    fn should_ellipse_before(&self, x_offset: f32) -> bool {
        self.x + self.style.letter_spacing + self.ellipsis_width + x_offset
            >= self.unscaled_bounds_size.x
    }

    fn next_position(&mut self, index: usize, character_size: Vec2) -> Vec2 {
        let mut width_used = if self.style.monospace {
            self.whitespace.x
        } else {
            character_size.x
        };

        width_used = width_used.max(self.style.min_glyph_width);

        if self.x + width_used > self.unscaled_bounds_size.x {
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

#[cfg(test)]
mod test {
    #[test]
    fn word_indices() {
        use super::word_indices;

        let expected = [
            (0, "A"),
            (1, " "),
            (2, "BC"),
            (4, " "),
            (5, " "),
            (6, "DEF"),
            (9, " "),
            (10, "G"),
        ];

        for (output, expected) in word_indices("A BC  DEF G").zip(expected) {
            assert_eq!(output, expected);
        }
    }
}
