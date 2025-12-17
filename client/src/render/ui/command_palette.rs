use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct CommandPalette<T: Copy + 'static> {
    label: String,
    label_9patch: NinePatch,
    body_9patch: NinePatch,
    arrow_animator: Animator,
    animation_time: FrameTime,
    scroll_tracker: ScrollTracker,
    options: Vec<(String, T)>,
    width: f32,
    open: bool,
}

impl<T: Copy + 'static> CommandPalette<T> {
    pub fn new(game_io: &GameIO, label_translation_key: &str) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // resources
        let texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);

        // styles
        let label_9patch = build_9patch!(game_io, texture.clone(), &animator, "CONTEXT_LABEL");
        let body_9patch = build_9patch!(game_io, texture, &animator, "CONTEXT_BODY");

        Self {
            label: globals.translate(label_translation_key),
            label_9patch,
            body_9patch,
            arrow_animator: Animator::load_new(assets, ResourcePaths::MORE_ARROWS_ANIMATION),
            animation_time: 0,
            scroll_tracker: ScrollTracker::new(game_io, 6).with_wrap(true),
            options: Default::default(),
            width: RESOLUTION_F.x * 0.7,
            open: false,
        }
    }

    pub fn with_width(mut self, width: f32) -> Self {
        self.width = width;
        self
    }

    pub fn with_options(mut self, options: Vec<(String, T)>) -> Self {
        self.set_options(options);
        self
    }

    pub fn set_options(&mut self, options: Vec<(String, T)>) {
        self.scroll_tracker.set_total_items(options.len());
        self.options = options;
    }

    pub fn is_open(&self) -> bool {
        self.open
    }

    pub fn open(&mut self) {
        self.open = true;
        self.scroll_tracker.set_selected_index(0);
    }

    pub fn close(&mut self) {
        self.open = false;
    }

    pub fn update(&mut self, game_io: &mut GameIO, ui_input_tracker: &UiInputTracker) -> Option<T> {
        if !self.open {
            return None;
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            // closed
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            self.open = false;
            return None;
        }

        self.scroll_tracker.handle_vertical_input(ui_input_tracker);

        if input_util.was_just_pressed(Input::Confirm) {
            self.options
                .get(self.scroll_tracker.selected_index())
                .map(|(_, command)| *command)
        } else {
            None
        }
    }

    pub fn get_label(&self, option_index: usize) -> Option<&str> {
        self.options
            .get(option_index)
            .map(|(label, _)| label.as_str())
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if !self.open {
            return;
        }

        // resolve bounds
        const BODY_TOP_PADDING: f32 = 3.0;

        let mut text_style = TextStyle::new(game_io, FontName::Context);
        let label_inner_height = text_style.line_height();
        let label_height = label_inner_height + self.label_9patch.heights();
        let mut height = label_height + BODY_TOP_PADDING;

        text_style.font = FontName::Thin;

        let line_step_y = text_style.line_height() + text_style.line_spacing;
        let options_height = line_step_y * self.scroll_tracker.view_range().len() as f32;
        height += options_height;

        let inner_size = Vec2::new(self.width, height);
        let top_left = (RESOLUTION_F - inner_size) * 0.5;
        let body_top_left = top_left + Vec2::new(0.0, label_height);

        self.scroll_tracker.define_cursor(
            body_top_left + Vec2::new(-8.0, BODY_TOP_PADDING + 1.0),
            line_step_y,
        );

        // render label
        let outer_width = self.width + self.body_9patch.widths() - self.label_9patch.widths();
        let label_top_left = top_left
            + Vec2::new(
                self.label_9patch.left_width() - self.body_9patch.left_width(),
                self.label_9patch.top_height(),
            );

        self.label_9patch.draw(
            sprite_queue,
            Rect::from_corners(
                label_top_left,
                label_top_left + Vec2::new(outer_width, label_inner_height),
            ),
        );

        text_style.font = FontName::Context;
        text_style.bounds.set_position(label_top_left);
        text_style.draw(game_io, sprite_queue, &self.label);

        // render body
        self.body_9patch.draw(
            sprite_queue,
            Rect::from_corners(body_top_left, top_left + inner_size),
        );

        // render items
        text_style.font = FontName::Thin;
        text_style.bounds.width = self.width;
        text_style.bounds.height = text_style.line_height();

        let mut position = body_top_left + Vec2::new(0.0, BODY_TOP_PADDING);

        for i in self.scroll_tracker.view_range() {
            text_style.bounds.set_position(position);

            position.y += text_style.line_height() + text_style.line_spacing;
            text_style.draw(game_io, sprite_queue, &self.options[i].0);
        }

        // render cursor
        self.scroll_tracker.draw_cursor(sprite_queue);

        // render arrows
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut arrow_sprite = assets.new_sprite(game_io, ResourcePaths::MORE_ARROWS);

        let mut draw_state = |sprite: &mut Sprite, state: &str| {
            self.arrow_animator.set_state(state);
            self.arrow_animator.set_loop_mode(AnimatorLoopMode::Loop);
            self.arrow_animator.sync_time(self.animation_time);
            self.arrow_animator.apply(sprite);
            sprite_queue.draw_sprite(sprite);
        };

        let mut arrow_position = body_top_left;
        arrow_position.x += self.width;

        let view_range = self.scroll_tracker.view_range();

        if view_range.start > 0 {
            arrow_sprite.set_position(arrow_position);
            draw_state(&mut arrow_sprite, "ARROW_UP");
        }

        if view_range.end < self.scroll_tracker.total_items() {
            arrow_position.y += options_height + BODY_TOP_PADDING + 2.0;
            arrow_sprite.set_position(arrow_position);
            draw_state(&mut arrow_sprite, "ARROW_DOWN");
        }

        self.animation_time += 1;
    }
}
