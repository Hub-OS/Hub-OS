use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct TextboxCursor {
    animator: Animator,
    sprite: Sprite,
}

impl TextboxCursor {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let mut animator = Animator::load_new(assets, ResourcePaths::TEXTBOX_CURSOR_ANIMATION);
        animator.set_state("DEFAULT");
        animator.set_loop_mode(AnimatorLoopMode::Loop);

        let sprite = assets.new_sprite(game_io, ResourcePaths::TEXTBOX_CURSOR);

        Self { animator, sprite }
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.sprite.set_position(position);
    }

    pub fn update(&mut self) {
        self.animator.update();
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.animator.apply(&mut self.sprite);
        sprite_queue.draw_sprite(&self.sprite);
    }
}
