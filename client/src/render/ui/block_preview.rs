use crate::{
    render::{Animator, SpriteColorQueue},
    resources::{AssetManager, Globals, ResourcePaths},
};
use framework::prelude::*;
use packets::structures::BlockColor;

pub struct BlockPreview {
    size: Vec2,
    position: Vec2,
    sprites: Vec<Sprite>,
}

impl BlockPreview {
    pub fn new(game_io: &GameIO, color: BlockColor, is_flat: bool, shape: [bool; 25]) -> Self {
        let mut sprites = Vec::new();

        let assets = &game_io.resource::<Globals>().unwrap().assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::CUSTOMIZE_UI);
        let mut animator = Animator::load_new(assets, ResourcePaths::CUSTOMIZE_PREVIEW_ANIMATION);

        // add background to sprites list
        animator.set_state("GRID");
        animator.apply(&mut sprite);

        let size = sprite.size();
        sprites.push(sprite.clone());

        // get constants for rendering shape
        let grid_start = animator.point("GRID_START").unwrap_or_default() - animator.origin();
        let grid_next = animator.point("GRID_STEP").unwrap_or_default();

        // set sprite to block for rendering shape
        animator.set_state(if is_flat {
            color.flat_state()
        } else {
            color.plus_state()
        });
        animator.apply(&mut sprite);

        for (i, shape) in shape.into_iter().enumerate() {
            if !shape {
                continue;
            }

            let row = i / 5;
            let col = i % 5;

            sprite.set_position(grid_start + grid_next * Vec2::new(col as f32, row as f32));
            sprites.push(sprite.clone());
        }

        Self {
            size,
            position: Vec2::ZERO,
            sprites,
        }
    }

    pub fn with_position(mut self, position: Vec2) -> Self {
        self.position = position;
        self
    }

    pub fn size(&self) -> Vec2 {
        self.size
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        for sprite in &mut self.sprites {
            let position = sprite.position();

            sprite.set_position(self.position + position);
            sprite_queue.draw_sprite(sprite);

            sprite.set_position(position);
        }
    }
}

impl Into<Vec<Sprite>> for BlockPreview {
    fn into(mut self) -> Vec<Sprite> {
        for sprite in &mut self.sprites {
            let position = sprite.position();

            sprite.set_position(self.position + position);
        }

        self.sprites
    }
}