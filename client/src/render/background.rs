use super::*;
use crate::resources::{AssetManager, Globals, ResourcePaths, RESOLUTION_F};
use framework::prelude::*;
use std::sync::Arc;

#[derive(Clone)]
pub struct Background {
    animator: Animator,
    sprite: Sprite,
    position: Vec2,
    offset: Vec2,
    velocity: Vec2,
}

impl Background {
    pub fn new(mut animator: Animator, sprite: Sprite) -> Self {
        if animator.has_state("BG") {
            animator.set_state("BG");
        } else if animator.has_state("DEFAULT") {
            animator.set_state("DEFAULT");
        }

        let velocity = animator.point("VELOCITY").unwrap_or_default();
        animator.set_loop_mode(AnimatorLoopMode::Loop);

        Self {
            animator,
            sprite,
            position: Vec2::ZERO,
            offset: Vec2::ZERO,
            velocity,
        }
    }

    pub fn new_static(sprite: Sprite) -> Self {
        Self::new(Animator::new(), sprite)
    }

    pub fn load_static(game_io: &GameIO, texture_path: &str) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;
        let sprite = assets.new_sprite(game_io, texture_path);
        Self::new_static(sprite)
    }

    pub fn new_blank(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        Self {
            animator: Animator::new(),
            sprite: Sprite::new(game_io, assets.texture(game_io, ResourcePaths::BLANK)),
            position: Vec2::ZERO,
            offset: Vec2::ZERO,
            velocity: Vec2::ZERO,
        }
    }

    pub fn new_sub_scene(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let animator = Animator::load_new(assets, ResourcePaths::SUB_SCENE_ANIMATION);
        let sprite = assets.new_sprite(game_io, ResourcePaths::SUB_SCENE);

        Self::new(animator, sprite)
    }

    pub fn animator(&self) -> &Animator {
        &self.animator
    }

    pub fn animator_mut(&mut self) -> &mut Animator {
        &mut self.animator
    }

    pub fn texture(&self) -> &Arc<Texture> {
        self.sprite.texture()
    }

    pub fn set_texture(&mut self, texture: Arc<Texture>) {
        self.sprite.set_texture(texture);
    }

    pub fn offset(&self) -> Vec2 {
        self.offset
    }

    pub fn set_offset(&mut self, offset: Vec2) {
        self.offset = offset;
    }

    pub fn velocity(&self) -> Vec2 {
        self.velocity
    }

    pub fn set_velocity(&mut self, velocity: Vec2) {
        self.velocity = velocity;
    }

    pub fn update(&mut self) {
        self.animator.update();
        self.animator.apply(&mut self.sprite);

        self.position += self.velocity;
    }

    pub fn draw(&self, game_io: &GameIO, render_pass: &mut RenderPass) {
        let pipeline = &game_io.resource::<Globals>().unwrap().background_pipeline;
        let mut queue = BackgroundQueue::new(game_io, pipeline);

        queue.draw_background(self);

        render_pass.consume_queue(queue);
    }
}

impl Instance<BackgroundInstanceData> for Background {
    fn instance_data(&self) -> BackgroundInstanceData {
        let texture_size = self.sprite.texture().size().as_vec2();

        let sprite_size = self.sprite.size();
        let scale = sprite_size / texture_size * (RESOLUTION_F / sprite_size);

        let mut offset = -(self.position + self.offset);

        if offset.x < 0.0 {
            offset.x %= sprite_size.x;
            offset.x += sprite_size.x;
        }

        if offset.y < 0.0 {
            offset.y %= sprite_size.y;
            offset.y += sprite_size.y;
        }

        // offset = offset.floor();
        offset.x /= RESOLUTION_F.x;
        offset.y /= RESOLUTION_F.y;

        let mut bounds = self.sprite.frame();
        bounds.x /= texture_size.x;
        bounds.y /= texture_size.y;
        bounds.width /= texture_size.x;
        bounds.height /= texture_size.y;

        // small shifts to remove seams
        let increment = 0.01 / texture_size.x;

        let x_increment = bounds.width.signum() * increment;
        bounds.x += x_increment;
        bounds.width -= x_increment;

        let y_increment = bounds.height.signum() * increment;
        bounds.y += y_increment;
        bounds.height -= y_increment;

        BackgroundInstanceData {
            scale,
            offset,
            bounds: bounds.into(),
        }
    }

    fn instance_resources(&self) -> Vec<Arc<dyn AsBinding>> {
        vec![self.sprite.texture().clone(), self.sprite.sampler().clone()]
    }
}

#[repr(C)]
#[derive(Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
pub struct BackgroundInstanceData {
    scale: Vec2,
    offset: Vec2,
    bounds: [f32; 4],
}

impl InstanceData for BackgroundInstanceData {
    fn instance_layout() -> InstanceLayout {
        InstanceLayout::new(&[
            VertexFormat::Float32x2,
            VertexFormat::Float32x2,
            VertexFormat::Float32x4,
        ])
    }
}
