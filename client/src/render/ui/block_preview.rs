use crate::{
    render::{Animator, SpriteColorQueue},
    resources::{AssetManager, Globals, ResourcePaths},
};
use framework::prelude::*;
use packets::structures::BlockColor;

pub struct BlockPreview {
    size: Vec2,
    scale: f32,
    position: Vec2,
    sprites: Vec<Sprite>,
}

impl BlockPreview {
    pub fn new(game_io: &GameIO, color: BlockColor, is_flat: bool, shape: [bool; 25]) -> Self {
        let mut sprites = Vec::new();

        let assets = &Globals::from_resources(game_io).assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::BLOCKS_UI);
        let mut animator = Animator::load_new(assets, ResourcePaths::BLOCKS_PREVIEW_ANIMATION);

        // add background to sprites list
        animator.set_state("GRID");
        animator.apply(&mut sprite);

        let size = sprite.size();
        sprites.push(sprite.clone());

        // get constants for rendering shape
        let grid_start = animator.point_or_zero("GRID_START") - animator.origin();
        let grid_next = animator.point_or_zero("GRID_STEP");

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
            scale: 1.0,
            position: Vec2::ZERO,
            sprites,
        }
    }

    pub fn add_multi_color_indicator(&mut self, game_io: &GameIO) {
        let assets = &Globals::from_resources(game_io).assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::BLOCKS_PREVIEW_ANIMATION);
        let mut sprite = assets.new_sprite(game_io, ResourcePaths::BLOCKS_UI);

        // add background to sprites list
        animator.set_state("MULTI_COLOR_INDICATOR");
        animator.apply(&mut sprite);
        self.sprites.push(sprite);
    }

    pub fn with_position(mut self, position: Vec2) -> Self {
        self.position = position;
        self
    }

    pub fn with_scale(mut self, scale: f32) -> Self {
        self.scale = scale;
        self
    }

    pub fn size(&self) -> Vec2 {
        self.size
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.position = position;
    }

    pub fn position(&self) -> Vec2 {
        self.position
    }

    pub fn set_tint(&mut self, color: Color) {
        for sprite in &mut self.sprites {
            sprite.set_color(color);
        }
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        for sprite in &mut self.sprites {
            let position = sprite.position();

            sprite.set_scale(Vec2::new(self.scale, self.scale));
            sprite.set_position(self.position + position * self.scale);
            sprite_queue.draw_sprite(sprite);

            sprite.set_position(position);
        }
    }
}

impl From<BlockPreview> for Vec<Sprite> {
    fn from(mut preview: BlockPreview) -> Vec<Sprite> {
        for sprite in &mut preview.sprites {
            let position = sprite.position();

            sprite.set_scale(Vec2::new(preview.scale, preview.scale));
            sprite.set_position(preview.position + position * preview.scale);
        }

        preview.sprites
    }
}
