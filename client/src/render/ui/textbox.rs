use super::{FontName, TextStyle};
use crate::bindable::SpriteColorMode;
use crate::render::ui::TextboxDoorstop;
use crate::render::ui::TextboxDoorstopKey;
use crate::render::ui::UiLayout;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use packets::structures::TextureAnimPathPair;
use std::borrow::Cow;
use std::collections::VecDeque;
use std::ops::Range;
use unicode_categories::UnicodeCategories;
use unicode_segmentation::UnicodeSegmentation;

const IDLE_CHAR_DELAY: FrameTime = 2;
const FAST_CHAR_DELAY: FrameTime = 1;
const IMMEDIATE_CHAR_DELAY: FrameTime = 0;
const DRAMATIC_CHAR_DELAY: FrameTime = 40;

pub trait TextboxInterface {
    /// This should not change after the first call
    fn text(&self) -> &str;

    fn hides_avatar(&self) -> bool {
        false
    }

    /// Show more indicator when animation ends and move on with input, ignoring is_complete()
    fn completes_with_indicator(&self) -> bool {
        false
    }

    /// Automatically move on to the next page without showing the more indicator
    fn auto_advance(&self) -> bool {
        false
    }

    fn is_complete(&self) -> bool;

    fn handle_completed(&mut self) {}

    /// Updates for custom ui. Called only when there's no more text
    fn update(&mut self, game_io: &mut GameIO, text_style: &TextStyle, lines: usize);

    fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue);
}

#[derive(Default, Clone)]
struct Page {
    lines: usize,
    range: Range<usize>,
}

pub struct Textbox {
    is_open: bool,
    accept_input: bool,
    hide_avatar: bool,
    avatar_queue: VecDeque<(Animator, Sprite, usize)>,
    animator: Animator,
    sprite: Sprite,
    next_animator: Animator,
    next_sprite: Sprite,
    next_sprite_offset: Vec2,
    interface_queue: VecDeque<(Box<dyn TextboxInterface>, Option<Box<TextStyle>>)>,
    page_queue: VecDeque<Page>,
    text_index: usize,
    char_time: FrameTime,
    text_offset: Vec2,
    text_style: TextStyle,
    transition_animation_enabled: bool,
    text_animation_enabled: bool,
    effect_processor: TextboxEffectProcessor,
    ime_offset: f32,
}

impl Textbox {
    fn new(game_io: &GameIO, texture_path: &str, animation_path: &str) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut animator = Animator::load_new(assets, animation_path);

        // first frame of IDLE contains text bounds information
        animator.set_state("IDLE");
        let text_bounds = Rect::from_corners(
            animator.point_or_zero("TEXT_TOP_LEFT") - animator.origin(),
            animator.point_or_zero("TEXT_BOTTOM_RIGHT") - animator.origin(),
        );

        let mut sprite = assets.new_sprite(game_io, texture_path);

        // calculate position
        animator.apply(&mut sprite);
        let bottom_height = sprite.size().y - animator.origin().y;
        let y = RESOLUTION_F.y - bottom_height;

        // get next_sprite_offset
        let next_sprite_offset = animator.point_or_zero("NEXT") - animator.origin();

        let textbox = Self {
            is_open: false,
            accept_input: true,
            hide_avatar: false,
            avatar_queue: VecDeque::new(),
            animator,
            sprite,
            next_animator: Animator::load_new(assets, ResourcePaths::TEXTBOX_NEXT_ANIMATION)
                .with_state("DEFAULT")
                .with_loop_mode(AnimatorLoopMode::Loop),
            next_sprite: assets.new_sprite(game_io, ResourcePaths::TEXTBOX_NEXT),
            next_sprite_offset,
            interface_queue: VecDeque::new(),
            page_queue: VecDeque::from([Page::default()]),
            text_index: 0,
            char_time: 0,
            text_offset: text_bounds.position(),
            text_style: TextStyle::new(game_io, FontName::Thin)
                .with_bounds(text_bounds)
                .with_color(TEXTBOX_TEXT_COLOR)
                .with_min_glyph_width(6.0)
                .with_line_spacing(3.0),
            transition_animation_enabled: true,
            text_animation_enabled: true,
            effect_processor: TextboxEffectProcessor::new(),
            ime_offset: 0.0,
        };

        textbox.with_position(Vec2::new(RESOLUTION_F.x * 0.5, y))
    }

    pub fn new_navigation(game_io: &GameIO) -> Self {
        let mut textbox = Self::new(
            game_io,
            ResourcePaths::NAVIGATION_TEXTBOX,
            ResourcePaths::NAVIGATION_TEXTBOX_ANIMATION,
        );

        textbox.use_navigation_avatar(game_io);
        textbox
    }

    pub fn new_overworld(game_io: &GameIO) -> Self {
        Self::new(
            game_io,
            ResourcePaths::OVERWORLD_TEXTBOX,
            ResourcePaths::OVERWORLD_TEXTBOX_ANIMATION,
        )
    }

    pub fn with_position(mut self, position: Vec2) -> Self {
        self.set_position(position);
        self
    }

    pub fn with_accept_input(mut self, accept_input: bool) -> Self {
        self.accept_input = accept_input;
        self
    }

    pub fn with_interface(mut self, interface: impl TextboxInterface + 'static) -> Self {
        self.push_interface(interface);
        self
    }

    pub fn position(&self) -> Vec2 {
        let mut position = self.sprite.position();
        position.y -= self.ime_offset;
        position
    }

    pub fn set_position(&mut self, mut position: Vec2) {
        position += self.ime_offset;
        self.sprite.set_position(position);
        self.text_style
            .bounds
            .set_position(position + self.text_offset);
        self.next_sprite
            .set_position(position + self.next_sprite_offset);
    }

    pub fn begin_open(mut self) -> Self {
        self.is_open = true;
        self.animator.set_state("IDLE");
        self
    }

    pub fn with_transition_animation_enabled(mut self, enable: bool) -> Self {
        self.transition_animation_enabled = enable;
        self
    }

    pub fn with_text_animation_enabled(mut self, enable: bool) -> Self {
        self.text_animation_enabled = enable;
        self
    }

    pub fn is_transition_animation_enabled(&self) -> bool {
        self.transition_animation_enabled
    }

    pub fn set_transition_animation_enabled(&mut self, enable: bool) {
        self.transition_animation_enabled = enable;
    }

    pub fn is_text_animation_enabled(&self) -> bool {
        self.text_animation_enabled
    }

    pub fn set_text_animation_enabled(&mut self, enable: bool) {
        self.text_animation_enabled = enable;
    }

    pub fn push_interface(&mut self, interface: impl TextboxInterface + 'static) {
        if let Some((_, _, count)) = self.avatar_queue.back_mut() {
            *count += 1;
        }

        self.interface_queue.push_back((Box::new(interface), None));

        if self.interface_queue.len() == 1 {
            self.create_pages();
        }
    }

    pub fn push_doorstop(&mut self) -> TextboxDoorstopKey {
        let (interface, key) = TextboxDoorstop::new();
        self.push_interface(interface);
        key
    }

    pub fn push_doorstop_with_message(&mut self, message: String) -> TextboxDoorstopKey {
        let (interface, key) = TextboxDoorstop::new();
        let interface = interface.with_string(message);
        self.push_interface(interface);
        key
    }

    pub fn is_complete(&self) -> bool {
        self.interface_queue.is_empty()
    }

    pub fn remaining_interfaces(&self) -> usize {
        self.interface_queue.len()
    }

    pub fn is_open(&self) -> bool {
        self.is_open
    }

    pub fn open(&mut self) {
        if self.is_open && !self.is_closing() {
            return;
        }

        if self.transition_animation_enabled {
            self.animator.set_state("OPEN");
        } else {
            self.animator.set_state("IDLE");
        }

        self.is_open = true;
    }

    pub fn is_closing(&self) -> bool {
        self.is_open && self.animator.current_state() == Some("CLOSE")
    }

    pub fn close(&mut self) {
        if !self.transition_animation_enabled {
            self.is_open = false;
        } else if !self.is_closing() {
            self.animator.set_state("CLOSE");
        }
    }

    pub fn use_blank_avatar(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        self.set_next_avatar(game_io, &globals.assets, Default::default());
    }

    pub fn use_navigation_avatar(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        let Some(player_package) = globals.global_save.player_package(game_io) else {
            self.set_next_avatar(
                game_io,
                &globals.assets,
                Some(&TextureAnimPathPair {
                    texture: Cow::Borrowed(ResourcePaths::GUIDE_MUG),
                    animation: Cow::Borrowed(ResourcePaths::GUIDE_MUG_ANIMATION),
                }),
            );
            return;
        };

        self.set_next_avatar(
            game_io,
            &globals.assets,
            Some(&player_package.mugshot_paths),
        );
    }

    pub fn set_next_avatar(
        &mut self,
        game_io: &GameIO,
        assets: &impl AssetManager,
        path_pair: Option<&TextureAnimPathPair>,
    ) {
        let (texture_path, animation_path) = path_pair
            .map(|pair| (&*pair.texture, &*pair.animation))
            .unwrap_or_default();

        let mut animator = Animator::load_new(assets, animation_path);
        animator.set_state("IDLE");
        animator.set_loop_mode(AnimatorLoopMode::Loop);

        let sprite = assets.new_sprite(game_io, texture_path);

        if let Some((_, _, 0)) = self.avatar_queue.back() {
            // no interface is using this avatar, we'll just drop it
            self.avatar_queue.pop_back();
        }

        self.avatar_queue.push_back((animator, sprite, 0));
    }

    /// A bit weird with set_next_avatar, maybe this or set_next_avatar should be refactored?
    pub fn set_last_text_style(&mut self, text_style: TextStyle) {
        let Some((_, optional_text_style)) = self.interface_queue.back_mut() else {
            return;
        };

        *optional_text_style = Some(Box::new(text_style));

        // recreate pages, maybe another reason to have this applied before setting the interface?
        self.create_pages();
    }

    pub fn update(&mut self, game_io: &mut GameIO) {
        // updating position based on ime height
        if game_io.input().accepting_text() {
            // get position with old ime offset stripped
            let mut position = self.position();

            // get new ime height
            let ime_height = UiLayout::corrected_ime_height(game_io);

            // resolve new offset, making sure to not send the editor out of bounds
            let raw_top = position.y - self.sprite.origin().y;
            let new_offset = (raw_top - ime_height).max(1.0) - raw_top;

            position.y += new_offset;

            self.ime_offset = 0.0;
            self.set_position(position);
            self.ime_offset = new_offset;
        } else if self.ime_offset != 0.0 {
            // reset position
            let position = self.position();
            self.ime_offset = 0.0;
            self.set_position(position);
        }

        if !self.is_open {
            return;
        }

        // update container animation

        self.animator.update();
        self.next_animator.update();

        if self.animator.is_complete() {
            match self.animator.current_state() {
                Some("OPEN") => {
                    self.animator.set_state("IDLE");
                    self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                }
                Some("CLOSE") => self.is_open = false,
                _ => {}
            }
        }

        if self.animator.current_state() == Some("IDLE") {
            if let Some((animator, _, _)) = self.avatar_queue.front_mut() {
                animator.update();
            }
        }

        if self.interface_queue.is_empty() {
            return;
        }

        // update text
        let input_util = InputUtil::new(game_io);
        let pressed_advance = self.accept_input && input_util.was_just_pressed(Input::Confirm);
        let pressed_skip =
            self.accept_input && (pressed_advance || input_util.was_just_pressed(Input::Cancel));

        let (auto_advance, completes_with_input) = (self.interface_queue.front())
            .map(|(interface, _)| {
                (
                    interface.auto_advance(),
                    interface.completes_with_indicator(),
                )
            })
            .unwrap_or_default();

        let page = self.page_queue.front().unwrap().clone();
        let is_last_page = self.page_queue.len() == 1;
        let page_completed = self.text_index == page.range.end;

        if !page_completed
            && (!self.text_animation_enabled || self.effect_processor.char_delay == 0)
        {
            self.skip_animation(game_io);
        } else if !page_completed {
            // try advancing character
            self.char_time += 1;

            if self.char_time >= self.effect_processor.char_delay {
                let text = match self.interface_queue.front() {
                    Some((interface, _)) => interface.text(),
                    None => "",
                };

                self.char_time = 0;
                self.text_index += grapheme_at(text, self.text_index).len();
                self.process_effects();
                self.update_avatar(game_io);
            }

            let requested_skip = pressed_skip || self.effect_processor.char_delay == 0;

            if requested_skip && self.text_index != page.range.end {
                // checked after testing for page completion to prevent Confirm from skipping + advancing same frame
                self.skip_animation(game_io);
            }
        } else {
            // try advancing page
            self.char_time += 1;

            if (!is_last_page || completes_with_input) && (pressed_advance || auto_advance) {
                self.advance_page(game_io);
            }
        }

        if let Some((interface, custom_style)) = self.interface_queue.front_mut() {
            let prev_completed = page_completed;
            let page_completed = self.text_index == page.range.end;

            if is_last_page && page_completed && prev_completed {
                let style = resolve_text_style(custom_style, &mut self.text_style);
                interface.update(game_io, style, page.lines);
            }

            self.hide_avatar = interface.hides_avatar();

            if interface.is_complete() {
                self.advance_interface(game_io);
            }
        }
    }

    fn process_effects(&mut self) {
        let text = match self.interface_queue.front() {
            Some((interface, _)) => interface.text(),
            None => "",
        };

        let page = self.page_queue.front().unwrap();

        while self.text_index < page.range.end {
            let current_char = char_at(text, self.text_index);

            if !self.effect_processor.process_char(current_char) {
                // no more effects
                break;
            }

            // skip the character, we shouldn't idle on it
            self.text_index += current_char.len_utf8();
        }
    }

    fn update_avatar(&mut self, game_io: &GameIO) {
        let text = match self.interface_queue.front() {
            Some((interface, _)) => interface.text(),
            None => "",
        };

        let current_char = char_at(text, self.text_index);

        let silent_char = current_char.is_control()
            || current_char.is_whitespace()
            || current_char.is_punctuation()
            || !self.text_animation_enabled
            || self.effect_processor.char_delay == 0;

        if !silent_char {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.text_blip);
        }

        let Some((animator, _, _)) = self.avatar_queue.front_mut() else {
            return;
        };

        let current_page = self.page_queue.front();
        let completed_speaking = !current_page.is_some_and(|page| self.text_index < page.range.end);
        let idle = !self.effect_processor.animate_avatar || silent_char || completed_speaking;

        let state = if idle { "IDLE" } else { "TALK" };

        if animator.current_state() != Some(state) {
            animator.set_state(state);
            animator.set_loop_mode(AnimatorLoopMode::Loop);
        }
    }

    pub fn skip_animation(&mut self, game_io: &GameIO) {
        let Some(page) = self.page_queue.front() else {
            return;
        };

        // process remaining effects
        let text = match self.interface_queue.front() {
            Some((interface, _)) => interface.text(),
            None => "",
        };

        while self.text_index < page.range.end {
            let current_char = char_at(text, self.text_index);
            self.effect_processor.process_char(current_char);
            self.text_index += current_char.len_utf8();
        }

        self.update_avatar(game_io);
    }

    fn advance_page(&mut self, game_io: &GameIO) {
        if self.page_queue.len() > 1 {
            self.page_queue.pop_front();
            self.text_index = self.page_queue.front().unwrap().range.start;
            self.char_time = 0;
            self.process_effects();
            self.update_avatar(game_io);
        } else {
            self.advance_interface(game_io);
        }
    }

    pub fn advance_interface(&mut self, game_io: &GameIO) {
        if let Some((mut interface, _)) = self.interface_queue.pop_front() {
            interface.handle_completed();

            if let Some((_, _, count)) = self.avatar_queue.front_mut() {
                *count = count.saturating_sub(1);

                if *count == 0 && self.avatar_queue.len() > 1 {
                    // drop the avatar if there's another waiting
                    self.avatar_queue.pop_front();
                }
            }
        }

        self.create_pages();

        if self.effect_processor.char_delay == 0 {
            self.skip_animation(game_io);
        } else {
            self.update_avatar(game_io);
        }
    }

    fn create_pages(&mut self) {
        let page_height = self.text_style.bounds.height;

        let (text, style) = match self.interface_queue.front_mut() {
            Some((interface, custom_style)) => (
                interface.text(),
                &*resolve_text_style(custom_style, &mut self.text_style),
            ),
            None => ("", &self.text_style),
        };

        let mut temp_style = style.clone();
        temp_style.bounds.height = f32::INFINITY;

        // todo: account for negative line spacing

        let lines_per_page = (page_height / style.line_height()).max(1.0) as usize;

        let metrics = temp_style.measure(text);

        let page_iter = metrics.line_ranges.chunks(lines_per_page).map(|lines| {
            let range = lines[0].start..lines.last().unwrap().end;

            Page {
                lines: lines.len(),
                range,
            }
        });

        self.text_index = 0;
        self.char_time = 0;
        self.page_queue.clear();
        self.page_queue.extend(page_iter);

        if self.page_queue.is_empty() {
            self.page_queue.push_back(Page::default());
        }

        self.effect_processor.reset();
        self.process_effects();
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if !self.is_open {
            return;
        }

        sprite_queue.set_color_mode(SpriteColorMode::Multiply);

        // render container
        self.animator.apply(&mut self.sprite);
        sprite_queue.draw_sprite(&self.sprite);

        if self.animator.current_state() != Some("IDLE") {
            // skip rendering since there's no textbox to sit on
            return;
        }

        if !self.hide_avatar {
            if let Some((animator, sprite, _)) = self.avatar_queue.front_mut() {
                // render avatar
                let point = self.animator.point_or_zero("AVATAR") - self.animator.origin();
                sprite.set_position(self.sprite.position() + point);

                animator.apply(sprite);
                sprite_queue.draw_sprite(sprite);
            }
        }

        if let Some((interface, custom_style)) = self.interface_queue.front_mut() {
            // render text
            if let Some(page) = self.page_queue.front() {
                let text = interface.text();
                let range = page.range.start..self.text_index;

                let style = resolve_text_style(custom_style, &mut self.text_style);

                style.draw_slice(game_io, sprite_queue, text, range)
            }

            // render extra
            interface.draw(game_io, sprite_queue);

            // rendering indicator
            let page = self.page_queue.front().unwrap();
            let is_last_page = self.page_queue.len() == 1;
            let page_completed = self.text_index == page.range.end;
            let page_ends_with_indicator = !is_last_page || interface.completes_with_indicator();

            if page_ends_with_indicator && page_completed {
                self.next_animator.apply(&mut self.next_sprite);
                sprite_queue.draw_sprite(&self.next_sprite);
            }
        }
    }
}

struct TextboxEffectProcessor {
    char_delay: FrameTime,
    animate_avatar: bool,
}

impl TextboxEffectProcessor {
    const DRAMATIC_TOKEN: char = '\x01';
    const NOLIP_TOKEN: char = '\x02';
    const FAST_TOKEN: char = '\x03';
    const IMMEDIATE_TOKEN: char = '\x04';

    fn new() -> Self {
        Self {
            char_delay: IDLE_CHAR_DELAY,
            animate_avatar: true,
        }
    }

    fn reset(&mut self) {
        *self = Self::new()
    }

    fn process_char(&mut self, char: char) -> bool {
        let mut new_speed = None;

        match char {
            Self::DRAMATIC_TOKEN => new_speed = Some(DRAMATIC_CHAR_DELAY),
            Self::FAST_TOKEN => new_speed = Some(FAST_CHAR_DELAY),
            Self::IMMEDIATE_TOKEN => new_speed = Some(IMMEDIATE_CHAR_DELAY),
            Self::NOLIP_TOKEN => self.animate_avatar = !self.animate_avatar,
            _ => {
                return false;
            }
        }

        if new_speed == Some(self.char_delay) {
            self.char_delay = IDLE_CHAR_DELAY;
        } else if let Some(speed) = new_speed {
            self.char_delay = speed;
        }

        true
    }
}

fn char_at(text: &str, index: usize) -> char {
    if index < text.len() {
        text[index..].chars().next().unwrap_or_default()
    } else {
        char::default()
    }
}

fn grapheme_at(text: &str, index: usize) -> &str {
    if index < text.len() {
        text[index..].graphemes(true).next().unwrap_or_default()
    } else {
        ""
    }
}

fn resolve_text_style<'a>(
    custom_style: &'a mut Option<Box<TextStyle>>,
    default_style: &'a mut TextStyle,
) -> &'a mut TextStyle {
    if let Some(style) = custom_style {
        style.bounds = default_style.bounds;
        &mut *style
    } else {
        default_style
    }
}
