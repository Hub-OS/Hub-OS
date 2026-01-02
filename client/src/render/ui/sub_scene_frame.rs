use crate::render::Animator;
use crate::render::SpriteColorQueue;
use crate::resources::AssetManager;
use crate::resources::Globals;
use crate::resources::ResourcePaths;
use framework::prelude::{GameIO, Sprite};

pub struct SubSceneFrame {
    animator: Animator,
    sprites: Vec<Sprite>,
}

impl SubSceneFrame {
    pub fn new(game_io: &GameIO) -> Self {
        let assets = &Globals::from_resources(game_io).assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::SUB_SCENE_ANIMATION);
        let mut sprite = assets.new_sprite(game_io, ResourcePaths::SUB_SCENE);

        animator.set_state("BOTTOM_BAR");
        animator.apply(&mut sprite);

        Self {
            animator,
            sprites: vec![sprite],
        }
    }

    pub fn with_left_arm(mut self, enabled: bool) -> Self {
        if !enabled {
            return self;
        }

        let mut sprite = self.sprites[0].clone();

        self.animator.set_state("LEFT_ARM");
        self.animator.apply(&mut sprite);

        self.sprites.push(sprite);
        self
    }

    pub fn with_right_arm(mut self, enabled: bool) -> Self {
        if !enabled {
            return self;
        }

        let mut sprite = self.sprites[0].clone();

        self.animator.set_state("RIGHT_ARM");
        self.animator.apply(&mut sprite);

        self.sprites.push(sprite);
        self
    }

    pub fn with_arms(self, enabled: bool) -> Self {
        if !enabled {
            return self;
        }

        self.with_left_arm(true).with_right_arm(true)
    }

    pub fn with_top_bar(mut self, enabled: bool) -> Self {
        if !enabled {
            return self;
        }

        let mut sprite = self.sprites[0].clone();

        self.animator.set_state("TOP_BAR");
        self.animator.apply(&mut sprite);

        self.sprites.push(sprite);
        self
    }

    pub fn with_everything(self, enabled: bool) -> Self {
        if !enabled {
            return self;
        }

        self.with_top_bar(true).with_arms(true)
    }

    pub fn draw(&self, sprite_queue: &mut SpriteColorQueue) {
        for sprite in &self.sprites {
            sprite_queue.draw_sprite(sprite);
        }
    }
}
