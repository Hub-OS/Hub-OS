use crate::ease::inverse_lerp;
use crate::render::{Animator, AnimatorLoopMode, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::saves::BlockGrid;
use framework::prelude::*;

const SETTLE_START_X: usize = BlockGrid::SIDE_LEN;
const SETTLE_END_X: usize = SETTLE_START_X + 9;
const FADE_START_X: usize = SETTLE_END_X + 1;
const FADE_END_X: usize = FADE_START_X + 2;

#[derive(PartialEq)]
pub enum GridArrowStatus {
    Block {
        position: (usize, usize),
        progress: f32,
    },
    Fading,
    Complete,
}

pub struct GridArrow {
    grid_start: Vec2,
    grid_step: Vec2,
    start: Vec2,
    offset_x: f32,
    max_width: f32,
    body_rect: Rect,
    body_sprites: Vec<Sprite>,
    tip_animator: Animator,
    tip_sprite: Sprite,
}

impl GridArrow {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::BLOCKS_UI_ANIMATION);

        // resolve start point
        animator.set_state("GRID");
        let grid_start = animator.point_or_zero("GRID_START") - animator.origin();
        let grid_step = animator.point_or_zero("GRID_STEP");
        let start = animator.point_or_zero("ARROW_START") - animator.origin();
        let end = animator.point_or_zero("ARROW_END") - animator.origin();

        // resolve width
        let max_width = end.x - start.x;

        // resolve sprites
        let mut sprite = assets.new_sprite(game_io, ResourcePaths::BLOCKS_UI);
        sprite.set_frame(Rect::ZERO);
        let mut body_sprites = Vec::new();

        animator.set_state("ARROW_BODY");
        let body_frame = animator.current_frame().cloned().unwrap_or_default();
        let body_rect = body_frame.bounds;
        let body_width = body_rect.width;

        let required_sprites = (max_width / body_width).ceil() as usize + 1;
        body_sprites.resize_with(required_sprites, || sprite.clone());

        // configure animator
        animator.set_state("ARROW_TIP");
        animator.set_loop_mode(AnimatorLoopMode::Loop);

        Self {
            grid_start,
            grid_step,
            start,
            offset_x: grid_start.x - start.x,
            max_width,
            body_rect,
            body_sprites,
            tip_animator: animator,
            tip_sprite: sprite,
        }
    }

    pub fn current_block(&self) -> (usize, usize) {
        let current_block = self.internal_current_block();

        (
            current_block.0.min(BlockGrid::SIDE_LEN - 1),
            current_block.1,
        )
    }

    fn internal_current_block(&self) -> (usize, usize) {
        let block_f = self.internal_current_block_f();

        (block_f.x as usize, block_f.y as usize)
    }

    fn internal_current_block_f(&self) -> Vec2 {
        let position = self.start + Vec2::new(self.offset_x, 0.0);
        (position - self.grid_start) / self.grid_step
    }

    pub fn status(&self) -> GridArrowStatus {
        let block_f = self.internal_current_block_f();
        let mut block = (block_f.x as usize, block_f.y as usize);

        match block.0 {
            0..=SETTLE_END_X => {
                let progress;

                if block.0 >= BlockGrid::SIDE_LEN {
                    block.0 = BlockGrid::SIDE_LEN - 1;
                    progress = 1.0;
                } else {
                    progress = block_f.x.fract();
                }

                GridArrowStatus::Block {
                    position: block,
                    progress,
                }
            }
            FADE_START_X..=FADE_END_X => GridArrowStatus::Fading,
            _ => GridArrowStatus::Complete,
        }
    }

    pub fn reset(&mut self) {
        self.offset_x = self.grid_start.x - self.start.x;

        for sprite in &mut self.body_sprites {
            sprite.set_frame(Rect::ZERO);
        }

        self.tip_sprite.set_frame(Rect::ZERO);
        self.tip_animator.sync_time(0);
    }

    pub fn update(&mut self) {
        let block = self.internal_current_block();

        self.update_offset();

        if block.0 >= SETTLE_START_X {
            // 2x updates
            self.update_offset();
        }

        self.update_opacity()
    }

    fn update_opacity(&mut self) {
        const DEFAULT_OPACITY: f32 = 0.75;

        let block_f = self.internal_current_block_f();

        let opacity = match block_f.x as usize {
            0..=SETTLE_END_X => DEFAULT_OPACITY,
            FADE_START_X..=FADE_END_X => {
                DEFAULT_OPACITY * inverse_lerp!(FADE_END_X, FADE_START_X, block_f.x)
            }
            _ => 0.0,
        };

        let color = Color::WHITE.multiply_alpha(opacity);

        for sprite in &mut self.body_sprites {
            sprite.set_color(color);
        }

        self.tip_sprite.set_color(color);
    }

    fn update_offset(&mut self) {
        self.offset_x += 1.0;

        if self.offset_x <= 0.0 {
            return;
        }

        let tip_frame = self.tip_animator.current_frame().cloned();
        let tip_frame = tip_frame.unwrap_or_default();
        let tip_width = tip_frame.bounds.width;

        // update tip_sprite
        if self.offset_x - tip_width > self.max_width {
            self.tip_animator.update();
            self.tip_animator.apply(&mut self.tip_sprite);
        } else {
            let mut rect = tip_frame.bounds;
            rect.width = rect.width.min(self.offset_x);
            rect.x += tip_frame.bounds.width - rect.width;

            self.tip_sprite.set_origin(tip_frame.origin);
            self.tip_sprite.set_frame(rect);
        }

        let tip_x = self.max_width.min(self.offset_x - tip_width);
        let tip_position = self.start + Vec2::new(tip_x, 0.0);
        self.tip_sprite.set_position(tip_position);

        // update loop sprites
        let mut offset_x = self.offset_x - tip_width;

        if offset_x > self.max_width {
            offset_x -= self.max_width;
            offset_x %= self.body_rect.width;
            offset_x += self.max_width;
        }

        for sprite in &mut self.body_sprites {
            Self::update_sprite(sprite, self.body_rect, self.start, self.max_width, offset_x);
            offset_x -= self.body_rect.width;
        }
    }

    fn update_sprite(
        sprite: &mut Sprite,
        frame_rect: Rect,
        start_position: Vec2,
        max_width: f32,
        offset: f32,
    ) {
        let mut rect = frame_rect;
        rect.x += (rect.width - offset).max(0.0);

        let left_trimmed_width = offset.min(rect.width);
        let right_trimmed_width = max_width + rect.width - offset;
        rect.width = left_trimmed_width.min(right_trimmed_width).max(0.0);

        sprite.set_frame(rect);

        let x = (offset - left_trimmed_width).max(0.0);
        let position = start_position + Vec2::new(x, 0.0);
        sprite.set_position(position);
    }

    pub fn draw(&self, sprite_queue: &mut SpriteColorQueue) {
        for sprite in &self.body_sprites {
            sprite_queue.draw_sprite(sprite);
        }

        sprite_queue.draw_sprite(&self.tip_sprite);
    }
}
