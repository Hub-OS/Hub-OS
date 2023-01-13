use framework::prelude::*;

pub struct ColorFadeTransition {
    start_instant: Option<Instant>,
    color: Color,
    duration: Duration,
    render_pipeline: FlatPipeline,
    model: FlatModel,
    camera: OrthoCamera,
}

impl ColorFadeTransition {
    pub fn new(game_io: &GameIO, color: Color, duration: Duration) -> Self {
        let mut camera = OrthoCamera::new(game_io, Vec2::ONE);
        camera.invert_y(false);

        let mesh = FlatModel::new_square_mesh();
        let model = FlatModel::new(mesh);

        Self {
            start_instant: None,
            color,
            duration,
            render_pipeline: FlatPipeline::new(game_io),
            model,
            camera,
        }
    }
}

impl Transition for ColorFadeTransition {
    fn draw(
        &mut self,
        game_io: &mut GameIO,
        render_pass: &mut RenderPass,
        previous_scene: &mut Box<dyn Scene>,
        next_scene: &mut Box<dyn Scene>,
    ) {
        let start_instant = match &self.start_instant {
            Some(instant) => instant,
            None => {
                self.start_instant = Some(game_io.frame_start_instant());
                self.start_instant.as_ref().unwrap()
            }
        };

        let mut progress = start_instant.elapsed().as_secs_f32() / self.duration.as_secs_f32();
        progress = progress.clamp(0.0, 1.0);

        // render scene
        if progress < 0.5 {
            previous_scene.draw(game_io, render_pass);
        } else {
            next_scene.draw(game_io, render_pass);
        }

        // render a flat shape to color the screen
        let render_pipeline = game_io.resource::<FlatPipeline>().unwrap();
        let mut flat_queue = RenderQueue::new(game_io, render_pipeline, [self.camera.as_binding()]);

        let mut color = self.color;

        // symmetric quart
        color.a = 1.0 - (progress * 2.0 - 1.0).powf(4.0);

        self.model.set_color(color);

        flat_queue.draw_model(&self.model);

        render_pass.consume_queue(flat_queue);
    }

    fn is_complete(&self) -> bool {
        self.start_instant
            .map(|instant| instant.elapsed() >= self.duration)
            .unwrap_or_default()
    }
}
