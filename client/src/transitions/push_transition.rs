use crate::bindable::Direction;
use framework::prelude::*;

pub struct PushTransition {
    start_instant: Option<Instant>,
    direction: Direction,
    duration: Duration,
    targets: Option<(RenderTarget, RenderTarget)>,
    camera: OrthoCamera,
}

impl PushTransition {
    /// * direction - The direction to enter from
    pub fn new(game_io: &GameIO, direction: Direction, duration: Duration) -> Self {
        let mut camera = OrthoCamera::new(game_io, Vec2::ONE);
        camera.invert_y(true);

        Self {
            start_instant: None,
            direction,
            duration,
            targets: None,
            camera,
        }
    }
}

impl Transition for PushTransition {
    fn draw(
        &mut self,
        game_io: &mut GameIO,
        render_pass: &mut RenderPass,
        previous_scene: &mut Box<dyn Scene>,
        next_scene: &mut Box<dyn Scene>,
    ) {
        let target_size = render_pass.target_size();

        // render scenes
        let current_size = self.targets.as_ref().map(|(a, _)| a.size());

        if current_size != Some(target_size) {
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
        let start_instant = self
            .start_instant
            .get_or_insert_with(|| game_io.frame_start_instant());

        let mut progress = start_instant.elapsed().as_secs_f32() / self.duration.as_secs_f32();
        progress = progress.clamp(0.0, 1.0);

        let b_offset: Vec2 = self.direction.chebyshev_vector().into();
        let vec = b_offset * progress;
        self.camera
            .set_position(Vec3::new(vec.x + 0.5, vec.y + 0.5, 0.0));

        // render transition
        let default_sprite_pipeline = game_io.resource::<DefaultSpritePipeline>().unwrap();
        let render_pipeline = default_sprite_pipeline.as_sprite_pipeline().clone();

        let mut sprite_queue =
            SpriteQueue::new(game_io, &render_pipeline, [self.camera.as_binding()])
                .with_inverted_y(true);

        let mut sprite_a = Sprite::new(game_io, target_a.texture().clone());
        sprite_a.set_size(Vec2::ONE);
        sprite_queue.draw_sprite(&sprite_a);

        let mut sprite_b = Sprite::new(game_io, target_b.texture().clone());
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
