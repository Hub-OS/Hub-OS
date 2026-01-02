use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct PageArrows {
    sprite: Sprite,
    animator: Animator,
}

impl PageArrows {
    pub fn new(game_io: &GameIO, position: Vec2) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::PAGE_ARROW);
        sprite.set_position(position);

        let mut animator = Animator::load_new(assets, ResourcePaths::PAGE_ARROW_ANIMATION);
        animator.set_state("default");
        animator.set_loop_mode(AnimatorLoopMode::Loop);

        Self { sprite, animator }
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.sprite.set_position(position);
    }

    pub fn update(&mut self) {
        self.animator.update();
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.animator.apply(&mut self.sprite);

        self.sprite.set_scale(Vec2::new(1.0, 1.0));
        sprite_queue.draw_sprite(&self.sprite);

        self.sprite.set_scale(Vec2::new(-1.0, 1.0));
        sprite_queue.draw_sprite(&self.sprite);
    }
}
