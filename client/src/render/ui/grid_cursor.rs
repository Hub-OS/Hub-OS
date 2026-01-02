use crate::render::{Animator, AnimatorLoopMode, FrameTime, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::*;

pub struct GridCursor {
    animator: Animator,
    sprite: Sprite,
    target_position: Vec2,
    time: FrameTime,
}

impl GridCursor {
    pub fn new(game_io: &GameIO) -> Self {
        let assets = &Globals::from_resources(game_io).assets;

        let animator = Animator::load_new(assets, ResourcePaths::BLOCKS_UI_ANIMATION)
            .with_state("TEXTBOX_CURSOR")
            .with_loop_mode(AnimatorLoopMode::Loop);

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::BLOCKS_UI);
        animator.apply(&mut sprite);

        Self {
            animator,
            sprite,
            target_position: Vec2::ZERO,
            time: 0,
        }
    }

    pub fn with_position(mut self, position: Vec2) -> Self {
        self.sprite.set_position(position);
        self.target_position = position;
        self
    }

    fn set_animator_state(&mut self, state: &str) {
        self.animator.set_state(state);
        self.animator.set_loop_mode(AnimatorLoopMode::Loop);
        self.animator.sync_time(self.time);
        self.sprite.set_color(Color::WHITE);
    }

    pub fn use_grid_cursor(&mut self) {
        self.set_animator_state("GRID_CURSOR");
    }

    pub fn use_textbox_cursor(&mut self) {
        self.set_animator_state("TEXTBOX_CURSOR");
    }

    pub fn use_claw_cursor(&mut self) {
        self.set_animator_state("CLAW_CURSOR");
    }

    pub fn use_claw_grip_cursor(&mut self) {
        self.set_animator_state("CLAW_GRIP");
    }

    pub fn hide(&mut self) {
        self.sprite.set_color(Color::TRANSPARENT);
    }

    pub fn target(&self) -> Vec2 {
        self.target_position
    }

    pub fn position(&self) -> Vec2 {
        self.sprite.position()
    }

    pub fn set_target(&mut self, position: Vec2) {
        self.target_position = position;
    }

    pub fn update(&mut self) {
        let mut final_position = self.sprite.position();
        final_position += (self.target_position - final_position) * 0.5;

        self.sprite.set_position(final_position);

        self.animator.update();
        self.animator.apply(&mut self.sprite);
        self.time += 1;
    }

    pub fn draw(&self, sprite_queue: &mut SpriteColorQueue) {
        sprite_queue.draw_sprite(&self.sprite);
    }
}
