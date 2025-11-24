use framework::prelude::*;

pub struct HoldColorScene {
    start_instant: Instant,
    duration: Duration,
    camera: OrthoCamera,
    model: FlatModel,
    next_scene_callback: Option<Box<dyn FnOnce(&GameIO) -> NextScene>>,
    next_scene: NextScene,
}

impl HoldColorScene {
    pub fn new(
        game_io: &GameIO,
        color: Color,
        duration: Duration,
        next_scene_callback: impl FnOnce(&GameIO) -> NextScene + 'static,
    ) -> Self {
        let mut camera = OrthoCamera::new(game_io, Vec2::ONE);
        camera.set_inverted_y(false);

        let mut model = FlatModel::new_square_model(game_io);
        model.set_color(color.to_linear());

        Self {
            start_instant: Instant::now(),
            duration,
            camera,
            model,
            next_scene: NextScene::None,
            next_scene_callback: Some(Box::new(next_scene_callback)),
        }
    }
}

impl Scene for HoldColorScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        if self.start_instant.elapsed() >= self.duration
            && let Some(callback) = self.next_scene_callback.take()
        {
            self.next_scene = callback(game_io);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let render_pipeline = game_io.resource::<FlatPipeline>().unwrap();
        let mut flat_queue = RenderQueue::new(game_io, render_pipeline, [self.camera.as_binding()]);

        flat_queue.draw_model(&self.model);
        render_pass.consume_queue(flat_queue);
    }
}
