use super::*;
use crate::resources::{AssetManager, Globals, RESOLUTION_F, ResourcePaths};
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
        if animator.current_state().is_none() {
            if animator.has_state("BG") {
                animator.set_state("BG");
            } else if animator.has_state("DEFAULT") {
                animator.set_state("DEFAULT");
            }
        }

        let velocity = animator.point_or_zero("VELOCITY");
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

    pub fn new_main_menu(game_io: &GameIO) -> Self {
        use chrono::Timelike;

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::MAIN_MENU_BG_ANIMATION);
        animator.set_state("DEFAULT");

        // resolve background using time of day
        let frame_0 = animator.frame(0).cloned().unwrap_or_default();
        let mut backgrounds: Vec<_> = frame_0
            .points
            .iter()
            .filter(|(label, _)| *label != "VELOCITY")
            .map(|(path, point)| (point.x as u32, path))
            .collect();

        if backgrounds.is_empty() {
            let sprite = assets.new_sprite(game_io, ResourcePaths::MAIN_MENU_BG);
            return Self::new(animator, sprite);
        }

        // sort for search
        backgrounds.sort_by_key(|(key, _)| *key);

        let time = chrono::Local::now().time();
        let time_u32 = time.hour() * 100 + time.minute();

        let index = match backgrounds.binary_search_by_key(&time_u32, |(key, _)| *key) {
            Ok(i) => i,
            Err(i) => {
                // Err path gives us an insertion index
                // we want to use the background before that index
                if i == 0 { backgrounds.len() - 1 } else { i - 1 }
            }
        };

        // resolve path
        let relative_path = backgrounds[index].1.as_str();
        let path = ResourcePaths::MAIN_MENU_ROOT.to_string() + relative_path;
        let cleaned_path = ResourcePaths::clean(&path);

        let animation_path = cleaned_path.clone() + ".animation";
        let sprite_path = cleaned_path + ".png";

        let animator = Animator::load_new(assets, &animation_path);
        let sprite = assets.new_sprite(game_io, &sprite_path);
        Self::new(animator, sprite)
    }

    pub fn new_sub_scene(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let animator = Animator::load_new(assets, ResourcePaths::SUB_SCENE_ANIMATION);
        let sprite = assets.new_sprite(game_io, ResourcePaths::SUB_SCENE);

        Self::new(animator, sprite)
    }

    pub fn new_character_scene(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let animator = Animator::load_new(assets, ResourcePaths::SUB_SCENE_ANIMATION)
            .with_state("CHARACTER_BG");
        let sprite = assets.new_sprite(game_io, ResourcePaths::SUB_SCENE);

        Self::new(animator, sprite)
    }

    pub fn new_battle(game_io: &GameIO) -> Self {
        match Self::randomized(game_io, ResourcePaths::BATTLE_BG_ANIMATION) {
            Ok(background) => background,
            Err(animator) => {
                let globals = game_io.resource::<Globals>().unwrap();
                let assets = &globals.assets;
                let background_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_BG);
                Self::new(animator, background_sprite)
            }
        }
    }

    pub fn new_credits(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let animator = Animator::load_new(assets, ResourcePaths::CREDITS_BG_ANIMATION)
            .with_state("CHARACTER_BG");
        let sprite = assets.new_sprite(game_io, ResourcePaths::CREDITS_BG);

        Self::new(animator, sprite)
    }

    #[allow(clippy::result_large_err)]
    fn randomized(game_io: &GameIO, animator_path: &str) -> Result<Self, Animator> {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut animator = Animator::load_new(assets, animator_path);
        animator.set_state("DEFAULT");

        let frame_0 = animator.frame(0).cloned().unwrap_or_default();

        let path_list: Vec<_> = frame_0
            .points
            .iter()
            .map(|(path, _)| path)
            .filter(|path| *path != "VELOCITY")
            .collect();

        let background_index = if cfg!(feature = "record_every_frame") || path_list.is_empty() {
            0
        } else {
            use rand::Rng;
            rand::rng().random_range(0..path_list.len())
        };

        let Some(background) = path_list.get(background_index) else {
            return Err(animator);
        };

        let path = ResourcePaths::parent(animator_path).unwrap().to_string() + background.as_str();

        Ok(Self::new(
            Animator::load_new(assets, &(path.clone() + ".animation")),
            assets.new_sprite(game_io, &(path + ".png")),
        ))
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
        bounds.width -= x_increment * 2.0;

        let y_increment = bounds.height.signum() * increment;
        bounds.y += y_increment;
        bounds.height -= y_increment * 2.0;

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
