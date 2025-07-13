use crate::bindable::SpriteColorMode;
use crate::render::{Camera, FrameTime, SpriteColorQueue, SpriteShaderEffect};
use crate::resources::{AssetManager, Globals, ResourcePaths, RESOLUTION_F};
use framework::prelude::*;

const MAX_PIXELATION: f32 = 1.0 / 8.0;

pub struct PixelateTransition {
    elapsed_frames: FrameTime,
    start_instant: Option<Instant>,
    duration: Duration,
    target: Option<RenderTarget>,
    camera: Camera,
}

impl PixelateTransition {
    pub fn new(game_io: &GameIO, duration: Duration) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        Self {
            elapsed_frames: 0,
            start_instant: None,
            duration,
            target: None,
            camera,
        }
    }

    fn calculate_white_intensity(&self, progress: f32) -> f32 {
        let linear_intensity = (progress * 2.0 - 1.0).abs();

        // transition during the first and last 15 frames, hold white in between
        const MAX_WHITE_TRANSITION: f32 = 15.0 / 60.0;
        let progress_relative_max = MAX_WHITE_TRANSITION / self.duration.as_secs_f32();
        let intensity_relative_max = progress_relative_max * 2.0;

        clamped_inverse_lerp!(1.0, 1.0 - intensity_relative_max, linear_intensity)
    }
}

impl SceneTransition for PixelateTransition {
    fn draw(
        &mut self,
        game_io: &mut GameIO,
        render_pass: &mut RenderPass,
        draw_previous_scene: &mut dyn FnMut(&mut GameIO, &mut RenderPass),
        draw_next_scene: &mut dyn FnMut(&mut GameIO, &mut RenderPass),
    ) {
        // calculate progress
        let start_instant = self
            .start_instant
            .get_or_insert_with(|| game_io.frame_start_instant());

        let mut progress = start_instant.elapsed().as_secs_f32() / self.duration.as_secs_f32();
        progress = progress.clamp(0.0, 1.0);

        // render scenes
        let target_size = render_pass.target_size();

        if self.target.as_ref().is_none_or(|t| t.size() != target_size) {
            self.target = Some(RenderTarget::new(game_io, target_size));
        }

        let target = self.target.as_ref().unwrap();

        let mut subpass = render_pass.create_subpass(target);

        if progress < 0.5 {
            draw_previous_scene(game_io, &mut subpass);
        } else {
            draw_next_scene(game_io, &mut subpass);
        }

        subpass.flush();

        // render transition
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        sprite_queue.set_shader_effect(SpriteShaderEffect::Pixelate);

        let white_intensity = self.calculate_white_intensity(progress);

        // render pixelated scene
        let mut target_sprite = Sprite::new(game_io, target.texture().clone());
        target_sprite.set_color(Color {
            a: 1.0 - white_intensity * MAX_PIXELATION,
            ..Color::WHITE
        });
        target_sprite.set_size(RESOLUTION_F);
        sprite_queue.draw_sprite(&target_sprite);

        // flash white on top
        let mut white_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        white_sprite.set_size(RESOLUTION_F);
        white_sprite.set_color(Color {
            a: white_intensity,
            ..Color::WHITE
        });
        sprite_queue.draw_sprite(&white_sprite);

        self.elapsed_frames += 1;

        render_pass.consume_queue(sprite_queue);
    }

    fn is_complete(&self) -> bool {
        self.start_instant
            .map(|instant| instant.elapsed() >= self.duration)
            .unwrap_or_default()
    }
}
