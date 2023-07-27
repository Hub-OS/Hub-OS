// not really a scene, but similar

use crate::render::Camera;
use crate::resources::{Globals, RESOLUTION_F};
use framework::prelude::*;
use std::collections::VecDeque;

pub struct DebugOverlay {
    camera: Camera,
    rectangle: FlatModel,
    history: VecDeque<f32>,
    last_key_pressed: Option<Key>,
}

const RECT_WIDTH: usize = 1;
const RECT_HEIGHT: usize = 16;
const ALPHA: f32 = 0.95;

impl DebugOverlay {
    pub fn new(game_io: &GameIO) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        let mut rectangle = FlatModel::new_square_model(game_io);
        rectangle.set_origin(Vec2::new(-0.5, 0.5));

        Self {
            camera,
            rectangle,
            history: VecDeque::new(),
            last_key_pressed: None,
        }
    }
}

impl GameOverlay for DebugOverlay {
    fn post_update(&mut self, game_io: &mut GameIO) {
        // update performance history
        let frame_duration = game_io.frame_duration();
        let target_duration = game_io.target_duration();
        let scale = frame_duration.as_secs_f32() / target_duration.as_secs_f32();

        self.history.push_back(scale);

        if self.history.len() > RESOLUTION_F.x as usize / RECT_WIDTH {
            self.history.pop_front();
        }

        // testing input
        let input = game_io.input();

        if input.latest_key().is_some() {
            self.last_key_pressed = input.latest_key();
        }

        // check to see if another key was pressed while F3 was held
        // if another key was pressed, assume a debug hotkey was pressed, such as F3 + V
        let toggling_debug =
            input.was_key_released(Key::F3) && self.last_key_pressed == Some(Key::F3);

        let globals = game_io.resource_mut::<Globals>().unwrap();

        if toggling_debug {
            globals.debug_visible = !globals.debug_visible;
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let globals = game_io.resource::<Globals>().unwrap();

        if !globals.debug_visible {
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
