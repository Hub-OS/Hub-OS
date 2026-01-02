use super::{build_9patch, FontName, NinePatch, TextStyle};
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::{GameIO, Rect, Sprite, Vec2};

pub struct ScrollableFrame {
    bounds: Rect,
    body_bounds: Rect,
    scroll_start: Vec2,
    scroll_end: Vec2,
    label_text_offset: Vec2,
    label_text: Option<String>,
    label_sprite: Sprite,
    nine_patch: NinePatch,
}

impl ScrollableFrame {
    pub fn new(game_io: &GameIO, bounds: Rect) -> Self {
        let assets = &Globals::from_resources(game_io).assets;
        let mut animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);

        // label
        let mut label_sprite = assets.new_sprite(game_io, ResourcePaths::UI_NINE_PATCHES);

        animator.set_state("LIST_LABEL");
        animator.apply(&mut label_sprite);
        let label_text_start = animator.point_or_zero("TEXT_START") - animator.origin();

        // frame
        let nine_patch = build_9patch!(game_io, label_sprite.texture().clone(), &animator, "LIST");

        let mut frame = Self {
            bounds,
            body_bounds: Rect::ZERO,
            scroll_start: Vec2::ZERO,
            scroll_end: Vec2::ZERO,
            label_text_offset: label_text_start,
            label_text: None,
            label_sprite,
            nine_patch,
        };

        frame.update_bounds(bounds);

        frame
    }

    pub fn with_label_str(mut self, label: &str) -> Self {
        self.label_text = Some(label.to_string());
        self
    }

    pub fn set_label(&mut self, label: String) {
        self.label_text = Some(label);
    }

    pub fn update_bounds(&mut self, bounds: Rect) {
        self.bounds = bounds;
        self.body_bounds = self.nine_patch.body_bounds(bounds);
        self.label_sprite.set_position(self.body_bounds.top_left());

        let right_frame = self.nine_patch.right_frame();

        // calculate scroll_start
        let start_offset = right_frame
            .and_then(|frame| frame.point("SCROLL_START"))
            .unwrap_or_default();

        self.scroll_start = self.body_bounds.top_right() + start_offset;

        // calculate scroll_end
        let end_offset = right_frame
            .and_then(|frame| frame.point("SCROLL_END"))
            .unwrap_or_default();

        self.scroll_end = self.body_bounds.bottom_right() + end_offset;
    }

    pub fn bounds(&self) -> Rect {
        self.bounds
    }

    pub fn body_bounds(&self) -> Rect {
        self.body_bounds
    }

    pub fn scroll_start(&self) -> Vec2 {
        self.scroll_start
    }

    pub fn scroll_end(&self) -> Vec2 {
        self.scroll_end
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        self.nine_patch.draw(sprite_queue, self.body_bounds);

        if let Some(text) = &self.label_text {
            sprite_queue.draw_sprite(&self.label_sprite);

            let label_position = self.label_sprite.position() + self.label_text_offset;

            let mut text_style = TextStyle::new(game_io, FontName::Micro);
            text_style.bounds.set_position(label_position);
            text_style.draw(game_io, sprite_queue, text);
        }
    }
}
