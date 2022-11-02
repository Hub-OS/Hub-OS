use crate::bindable::Direction;
use framework::prelude::*;
use std::sync::Arc;

pub struct PushTransition {
    start_instant: Option<Instant>,
    direction: Direction,
    duration: Duration,
    targets: Option<(RenderTarget, RenderTarget)>,
    render_pipeline: SpritePipeline<SpriteInstanceData>,
    sampler: Arc<TextureSampler>,
    camera: OrthoCamera,
}

impl PushTransition {
    /// * direction - The direction to enter from
    pub fn new<Globals>(
        game_io: &GameIO<Globals>,
        sampler: Arc<TextureSampler>,
        direction: Direction,
        duration: Duration,
    ) -> Self {
        let mut camera = OrthoCamera::new(game_io, Vec2::ONE);
        camera.invert_y(true);

        Self {
            start_instant: None,
            direction,
            duration,
            targets: None,
            sampler,
            render_pipeline: SpritePipeline::new(game_io, true),
            camera,
        }
    }
}

impl<Globals> Transition<Globals> for PushTransition {
    fn draw(
        &mut self,
        game_io: &mut GameIO<Globals>,
        render_pass: &mut RenderPass,
        previous_scene: &mut Box<dyn Scene<Globals>>,
        next_scene: &mut Box<dyn Scene<Globals>>,
    ) {
        let target_size = render_pass.target_size();

        // render scenes
        let current_size = self
            .targets
            .as_ref()
            .map(|(a, _)| a.size())
            .unwrap_or_default();

        if current_size != target_size {
            self.targets = Some((
                RenderTarget::new(game_io, target_size),
                RenderTarget::new(game_io, target_size),
            ));
        }

        let (target_a, target_b) = self.targets.as_ref().unwrap();

        let mut subpass = render_pass.create_subpass(target_a);
        previous_scene.draw(game_io, &mut subpass);
        subpass.flush();

        let mut subpass = render_pass.create_subpass(target_b);
        next_scene.draw(game_io, &mut subpass);
        subpass.flush();

        // calculate camera offset
        let start_instant = match &self.start_instant {
            Some(instant) => instant,
            None => {
                self.start_instant = Some(game_io.frame_start_instant());
                self.start_instant.as_ref().unwrap()
            }
        };

        let mut progress = start_instant.elapsed().as_secs_f32() / self.duration.as_secs_f32();
        progress = progress.clamp(0.0, 1.0);

        let b_offset: Vec2 = self.direction.chebyshev_vector().into();
        let vec = b_offset * progress;
        self.camera
            .set_position(Vec3::new(vec.x + 0.5, vec.y + 0.5, 0.0));

        // render transition
        let mut sprite_queue =
            SpriteQueue::new(game_io, &self.render_pipeline, [self.camera.as_binding()]);

        let mut sprite_a = Sprite::new(target_a.texture().clone(), self.sampler.clone());
        sprite_a.set_size(Vec2::ONE);
        sprite_queue.draw_sprite(&sprite_a);

        let mut sprite_b = Sprite::new(target_b.texture().clone(), self.sampler.clone());
        sprite_b.set_size(Vec2::ONE);
        sprite_b.set_position(b_offset);
        sprite_queue.draw_sprite(&sprite_b);

        render_pass.consume_queue(sprite_queue);
    }

    fn is_complete(&self) -> bool {
        self.start_instant
            .map(|instant| instant.elapsed() >= self.duration)
            .unwrap_or_default()
    }
}
