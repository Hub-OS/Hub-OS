use crate::render::{Animator, AnimatorLoopMode, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::math::{Rect, Vec2};
use framework::{common::GameIO, graphics::Sprite};

pub struct UiConfigCycleArrows {
    sprite: Sprite,
    animator: Animator,
}

impl UiConfigCycleArrows {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let sprite = assets.new_sprite(game_io, ResourcePaths::PAGE_ARROW);

        let mut animator = Animator::load_new(assets, ResourcePaths::PAGE_ARROW_ANIMATION);
        animator.set_state("default");
        animator.set_loop_mode(AnimatorLoopMode::Loop);

        Self { sprite, animator }
    }

    pub fn update(&mut self) {
        self.animator.update();
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue, bounds: Rect) {
        self.animator.apply(&mut self.sprite);

        let origin = self.sprite.origin();
        self.sprite.set_origin(Vec2::new(-1.0, origin.y));

        self.sprite.set_position(bounds.center_left().floor());
        self.sprite.set_scale(Vec2::new(-1.0, 1.0));
        sprite_queue.draw_sprite(&self.sprite);

        self.sprite.set_position(bounds.center_right().floor());
        self.sprite.set_scale(Vec2::new(1.0, 1.0));
        sprite_queue.draw_sprite(&self.sprite);
    }
}
