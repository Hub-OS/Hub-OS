use crate::bindable::Direction;
use framework::prelude::*;
use std::sync::Arc;

pub struct SwipeInTransition {
    start_instant: Option<Instant>,
    direction: Direction,
    duration: Duration,
    targets: Option<(RenderTarget, RenderTarget)>,
    render_pipeline: SpritePipeline<SpriteInstanceData>,
    sampler: Arc<TextureSampler>,
    camera: OrthoCamera,
}

impl SwipeInTransition {
    pub fn new<Globals>(
        game_io: &mut GameIO<Globals>,
        sampler: Arc<TextureSampler>,
        direction: Direction,
        duration: Duration,
    ) -> Self {
        let mut camera = OrthoCamera::new(game_io, Vec2::ONE);
        camera.invert_y(true);
        camera.set_position(Vec3::new(0.5, 0.5, 0.0));

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

impl<Globals> Transition<Globals> for SwipeInTransition {
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

        let (target_a, target_b) = self.targets.as_mut().unwrap();

        let clear_color = render_pass.clear_color();
        target_a.set_clear_color(clear_color);
        target_b.set_clear_color(clear_color);

        let mut subpass = render_pass.create_subpass(target_a);
        next_scene.draw(game_io, &mut subpass);
        subpass.flush();

        let mut subpass = render_pass.create_subpass(target_b);
        previous_scene.draw(game_io, &mut subpass);
        subpass.flush();

        // calculate camera offset
        let start_instant = match &self.start_instant {
            Some(instant) => instant,
            None => {
                self.start_instant = Some(game_io.frame_start_instant());
                self.start_instant.as_ref().unwrap()
            }
        };

        // render transition
        let mut sprite_queue =
            SpriteQueue::new(game_io, &self.render_pipeline, [self.camera.as_binding()]);

        let mut sprite_a = Sprite::new(target_a.texture().clone(), self.sampler.clone());
        sprite_a.set_size(Vec2::ONE);
        sprite_queue.draw_sprite(&sprite_a);

        let mut sprite_b = Sprite::new(target_b.texture().clone(), self.sampler.clone());

        let mut progress = start_instant.elapsed().as_secs_f32() / self.duration.as_secs_f32();
        progress = progress.clamp(0.0, 1.0);

        let progress_inv = 1.0 - progress;
        let (w, h) = target_size.as_vec2().into();

        match self.direction {
            Direction::Left => {
                sprite_b.set_frame(Rect::new(0.0, 0.0, w * progress_inv, h));
                sprite_b.set_size(Vec2::new(progress_inv, 1.0));
                sprite_b.set_position(Vec2::new(progress, 0.0));
            }
            Direction::Right => {
                sprite_b.set_frame(Rect::new(w * progress, 0.0, w * progress_inv, h));
                sprite_b.set_size(Vec2::new(progress_inv, 1.0));
            }
            Direction::Up => {
                sprite_b.set_frame(Rect::new(0.0, 0.0, w, h * progress_inv));
                sprite_b.set_size(Vec2::new(progress_inv, 1.0));
            }
            Direction::Down => {
                sprite_b.set_frame(Rect::new(0.0, h * progress, w, h * progress_inv));
                sprite_b.set_size(Vec2::new(1.0, progress_inv));
                sprite_b.set_position(Vec2::new(0.0, progress));
            }
            _ => {}
        }

        sprite_queue.draw_sprite(&sprite_b);

        render_pass.consume_queue(sprite_queue);
    }

    fn is_complete(&self) -> bool {
        self.start_instant
            .map(|instant| instant.elapsed() >= self.duration)
            .unwrap_or_default()
    }
}
