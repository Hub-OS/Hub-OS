// not really a scene, but similar

use crate::render::Camera;
use crate::resources::{Globals, RESOLUTION_F};
use framework::prelude::*;
use std::collections::VecDeque;

pub struct Overlay {
    camera: Camera,
    rectangle: FlatModel,
    history: VecDeque<f32>,
    visible: bool,
}

const RECT_WIDTH: usize = 1;
const RECT_HEIGHT: usize = 16;
const ALPHA: f32 = 0.95;

impl Overlay {
    pub fn new(game_io: &GameIO) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        let mut rectangle = FlatModel::new_square_model();
        rectangle.set_origin(Vec2::new(-0.5, 0.5));

        Self {
            camera,
            rectangle,
            history: VecDeque::new(),
            visible: false,
        }
    }
}

impl SceneOverlay for Overlay {
    fn update(&mut self, game_io: &mut GameIO) {
        let frame_duration = game_io.frame_duration();
        let target_duration = game_io.target_duration();
        let scale = frame_duration.as_secs_f32() / target_duration.as_secs_f32();

        self.history.push_back(scale);

        if self.history.len() > RESOLUTION_F.x as usize / RECT_WIDTH {
            self.history.pop_front();
        }

        if game_io.input().was_key_just_pressed(Key::F3) {
            self.visible = !self.visible;
        }
        game_io.resource_mut::<Globals>().unwrap().tick();
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        if !self.visible {
            return;
        }

        let render_pipeline = game_io.resource::<FlatPipeline>().unwrap();
        let mut queue = RenderQueue::new(game_io, render_pipeline, [self.camera.as_binding()]);

        // draw history
        for (i, scale) in self.history.iter().enumerate() {
            let height = *scale * RECT_HEIGHT as f32;

            self.rectangle
                .set_position(Vec2::new((i * RECT_WIDTH) as f32, RESOLUTION_F.y));
            self.rectangle
                .set_scale(Vec2::new(RECT_WIDTH as f32, height));

            let mut color = if *scale > 1.0 {
                Color::lerp(Color::GREEN, Color::RED, (*scale - 1.0).min(1.0))
            } else {
                Color::GREEN
            };

            color.a = ALPHA;

            self.rectangle.set_color(color);

            queue.draw_model(&self.rectangle);
        }

        // draw target fps line
        let line_color = Color::WHITE.multiply_alpha(ALPHA);

        self.rectangle
            .set_position(Vec2::new(0.0, RESOLUTION_F.y - (RECT_HEIGHT as f32)));
        self.rectangle.set_scale(Vec2::new(RESOLUTION_F.x, 1.0));
        self.rectangle.set_color(line_color);
        queue.draw_model(&self.rectangle);

        render_pass.consume_queue(queue);
    }
}
