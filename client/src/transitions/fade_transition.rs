use framework::prelude::*;

pub struct FadeTransition {
    start_instant: Option<Instant>,
    duration: Duration,
    model: FlatModel,
    prev_scene_target: Option<RenderTarget>,
    camera: OrthoCamera,
}

impl FadeTransition {
    pub fn new(game_io: &GameIO, duration: Duration) -> Self {
        let mut camera = OrthoCamera::new(game_io, Vec2::ONE);
        camera.set_position(Vec3::new(0.5, 0.5, 0.0));
        camera.invert_y(false);

        let model = FlatModel::new_square_model(game_io);

        Self {
            start_instant: None,
            duration,
            model,
            prev_scene_target: None,
            camera,
        }
    }
}

impl SceneTransition for FadeTransition {
    fn draw(
        &mut self,
        game_io: &mut GameIO,
        render_pass: &mut RenderPass,
        draw_previous_scene: &mut dyn FnMut(&mut GameIO, &mut RenderPass),
        draw_next_scene: &mut dyn FnMut(&mut GameIO, &mut RenderPass),
    ) {
        // render the next scene first to display under the effect
        draw_next_scene(game_io, render_pass);

        // render previous scene to render target
        let target_size = render_pass.target_size();
        let current_size = self.prev_scene_target.as_ref().map(|target| target.size());

        if current_size != Some(target_size) {
            self.prev_scene_target = Some(RenderTarget::new(game_io, target_size));
        }

        let prev_scene_target = self.prev_scene_target.as_ref().unwrap();

        let mut subpass = render_pass.create_subpass(prev_scene_target);
        draw_previous_scene(game_io, &mut subpass);
        subpass.flush();

        // calculate progress
        let start_instant = self
            .start_instant
            .get_or_insert_with(|| game_io.frame_start_instant());

        let mut progress = start_instant.elapsed().as_secs_f32() / self.duration.as_secs_f32();
        progress = progress.clamp(0.0, 1.0);

        // render the target with changing transparency
        let default_sprite_pipeline = game_io.resource::<DefaultSpritePipeline>().unwrap();
        let render_pipeline = default_sprite_pipeline.as_sprite_pipeline();

        let mut sprite_queue =
            SpriteQueue::new(game_io, render_pipeline, [self.camera.as_binding()]);

        let mut sprite_a = Sprite::new(game_io, prev_scene_target.texture().clone());
        sprite_a.set_size(Vec2::ONE);
        sprite_a.set_color(Color::WHITE.multiply_alpha(1.0 - progress));
        sprite_queue.draw_sprite(&sprite_a);

        render_pass.consume_queue(sprite_queue);
    }

    fn is_complete(&self) -> bool {
        self.start_instant
            .map(|instant| instant.elapsed() >= self.duration)
            .unwrap_or_default()
    }
}
