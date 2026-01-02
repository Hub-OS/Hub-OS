use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::sync::Arc;

#[derive(Clone)]
pub struct NinePatch {
    sprite: Sprite,
    body: Option<AnimationFrame>,
    top_left: Option<AnimationFrame>,
    top_right: Option<AnimationFrame>,
    bottom_left: Option<AnimationFrame>,
    bottom_right: Option<AnimationFrame>,
    left: Option<AnimationFrame>,
    right: Option<AnimationFrame>,
    top: Option<AnimationFrame>,
    bottom: Option<AnimationFrame>,
    // the rest is not implemented as we're not using it yet
}

impl NinePatch {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);

        Self {
            sprite: globals.assets.new_sprite(game_io, ResourcePaths::BLANK),
            body: None,
            top_left: None,
            top_right: None,
            bottom_left: None,
            bottom_right: None,
            left: None,
            right: None,
            top: None,
            bottom: None,
        }
    }

    pub fn set_texture(&mut self, texture: Arc<Texture>) {
        self.sprite.set_texture(texture);
    }

    pub fn set_color(&mut self, color: Color) {
        self.sprite.set_color(color);
    }

    pub fn body_frame(&self) -> Option<&AnimationFrame> {
        self.body.as_ref()
    }

    pub fn set_body(&mut self, animator: &Animator, state: &'static str) {
        self.body = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn set_top_left(&mut self, animator: &Animator, state: &'static str) {
        self.top_left = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn set_top_right(&mut self, animator: &Animator, state: &'static str) {
        self.top_right = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn set_bottom_left(&mut self, animator: &Animator, state: &'static str) {
        self.bottom_left = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn set_bottom_right(&mut self, animator: &Animator, state: &'static str) {
        self.bottom_right = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn left_width(&self) -> f32 {
        self.left
            .as_ref()
            .map(|frame| frame.origin.x)
            .unwrap_or_default()
    }

    pub fn left_frame(&self) -> Option<&AnimationFrame> {
        self.left.as_ref()
    }

    pub fn set_left(&mut self, animator: &Animator, state: &'static str) {
        self.left = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn right_width(&self) -> f32 {
        self.right
            .as_ref()
            .map(|frame| frame.bounds.width - frame.origin.x)
            .unwrap_or_default()
    }

    pub fn right_frame(&self) -> Option<&AnimationFrame> {
        self.right.as_ref()
    }

    pub fn set_right(&mut self, animator: &Animator, state: &'static str) {
        self.right = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn widths(&self) -> f32 {
        self.left_width() + self.right_width()
    }

    pub fn top_height(&self) -> f32 {
        self.top
            .as_ref()
            .map(|frame| frame.origin.y)
            .unwrap_or_default()
    }

    pub fn top_frame(&self) -> Option<&AnimationFrame> {
        self.top.as_ref()
    }

    pub fn set_top(&mut self, animator: &Animator, state: &'static str) {
        self.top = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn bottom_height(&self) -> f32 {
        self.bottom
            .as_ref()
            .map(|frame| frame.bounds.height - frame.origin.y)
            .unwrap_or_default()
    }

    pub fn bottom_frame(&self) -> Option<&AnimationFrame> {
        self.bottom.as_ref()
    }

    pub fn set_bottom(&mut self, animator: &Animator, state: &'static str) {
        self.bottom = animator
            .frame_list(state)
            .and_then(|frame_list| frame_list.frame(0).cloned());
    }

    pub fn heights(&self) -> f32 {
        self.top_height() + self.bottom_height()
    }

    pub fn body_bounds(&self, mut rect: Rect) -> Rect {
        let left = self.left_width();
        let right = self.right_width();

        rect.x += left;
        rect.width = (rect.width - left - right).max(0.0);

        let top = self.top_height();
        let bottom = self.bottom_height();

        rect.y += top;
        rect.height = (rect.height - top - bottom).max(0.0);

        rect
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue, rect: Rect) {
        let top_left = rect.top_left();
        let top_right = rect.top_right();
        let bottom_left = rect.bottom_left();

        if let Some(frame) = &self.body {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(top_left);
            self.sprite.set_size(rect.size());
            sprite_queue.draw_sprite(&self.sprite);
        }

        if let Some(frame) = &self.left {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(top_left);
            self.sprite.set_scale(Vec2::ONE);
            self.sprite.set_height(rect.height);
            sprite_queue.draw_sprite(&self.sprite);
        }

        if let Some(frame) = &self.right {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(top_right);
            self.sprite.set_scale(Vec2::ONE);
            self.sprite.set_height(rect.height);
            sprite_queue.draw_sprite(&self.sprite);
        }

        if let Some(frame) = &self.top {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(top_left);
            self.sprite.set_scale(Vec2::ONE);
            self.sprite.set_width(rect.width);
            sprite_queue.draw_sprite(&self.sprite);
        }

        if let Some(frame) = &self.top_left {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(top_left);
            self.sprite.set_scale(Vec2::ONE);
            sprite_queue.draw_sprite(&self.sprite);
        }

        if let Some(frame) = &self.top_right {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(top_right);
            self.sprite.set_scale(Vec2::ONE);
            sprite_queue.draw_sprite(&self.sprite);
        }

        if let Some(frame) = &self.bottom {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(bottom_left);
            self.sprite.set_scale(Vec2::ONE);
            self.sprite.set_width(rect.width);
            sprite_queue.draw_sprite(&self.sprite);
        }

        if let Some(frame) = &self.bottom_left {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(bottom_left);
            self.sprite.set_scale(Vec2::ONE);
            sprite_queue.draw_sprite(&self.sprite);
        }

        if let Some(frame) = &self.bottom_right {
            frame.apply(&mut self.sprite);

            self.sprite.set_position(rect.bottom_right());
            self.sprite.set_scale(Vec2::ONE);
            sprite_queue.draw_sprite(&self.sprite);
        }
    }
}

macro_rules! build_9patch {
    ($game_io:expr, $texture:expr, $animator:expr, $state_prefix:literal) => {{
        use crate::render::ui::NinePatch;

        let mut nine_patch = NinePatch::new($game_io);
        let animator = $animator;

        nine_patch.set_texture($texture);
        nine_patch.set_body(animator, &concat!($state_prefix, "_BODY"));
        nine_patch.set_top_left(animator, &concat!($state_prefix, "_TOP_LEFT"));
        nine_patch.set_top_right(animator, &concat!($state_prefix, "_TOP_RIGHT"));
        nine_patch.set_bottom_left(animator, &concat!($state_prefix, "_BOTTOM_LEFT"));
        nine_patch.set_bottom_right(animator, &concat!($state_prefix, "_BOTTOM_RIGHT"));
        nine_patch.set_left(animator, &concat!($state_prefix, "_LEFT"));
        nine_patch.set_right(animator, &concat!($state_prefix, "_RIGHT"));
        nine_patch.set_top(animator, &concat!($state_prefix, "_TOP"));
        nine_patch.set_bottom(animator, &concat!($state_prefix, "_BOTTOM"));

        nine_patch
    }};
}

pub(crate) use build_9patch;
