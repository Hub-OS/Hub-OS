use crate::bindable::SpriteColorMode;
use crate::render::ui::{FontName, Text};
use crate::render::{Camera, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths, RESOLUTION_F};
use framework::prelude::*;
use std::collections::VecDeque;

pub struct DebugOverlay {
    camera: Camera,
    history: VecDeque<f32>,
    last_key_pressed: Option<Key>,
}

const RECT_WIDTH: usize = 1;
const RECT_HEIGHT: usize = 16;
const RECT_WIDTH_F: f32 = RECT_WIDTH as f32;
const RECT_HEIGHT_F: f32 = RECT_HEIGHT as f32;
const ALPHA: f32 = 0.95;

impl DebugOverlay {
    pub fn new(game_io: &GameIO) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        Self {
            camera,
            history: VecDeque::new(),
            last_key_pressed: None,
        }
    }

    fn detect_debug_hotkeys(&self, game_io: &GameIO) {
        let input = game_io.input();

        if !input.is_key_down(Key::F3) {
            return;
        }

        if input.was_key_just_pressed(Key::N) {
            let globals = game_io.resource::<Globals>().unwrap();
            let mut namespaces = globals.namespaces().collect::<Vec<_>>();
            namespaces.sort();

            let namespace_list_string = namespaces
                .into_iter()
                .map(|ns| format!("  {ns:?}"))
                .collect::<Vec<_>>()
                .join("\n");

            println!("Loaded Namespaces:\n{}", namespace_list_string);
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

        self.detect_debug_hotkeys(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let globals = game_io.resource::<Globals>().unwrap();

        if !globals.debug_visible {
            return;
        }

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);
        let assets = &globals.assets;
        let mut rectangle = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);

        // draw history
        for (i, scale) in self.history.iter().enumerate() {
            let height = *scale * RECT_HEIGHT_F;

            rectangle.set_position(Vec2::new((i * RECT_WIDTH) as f32, RESOLUTION_F.y - height));
            rectangle.set_scale(Vec2::new(RECT_WIDTH_F, height));

            let mut color = if *scale > 1.0 {
                Color::lerp(Color::GREEN, Color::RED, (*scale - 1.0).min(1.0))
            } else {
                Color::GREEN
            };

            color.a = ALPHA;

            rectangle.set_color(color);

            sprite_queue.draw_sprite(&rectangle);
        }

        // draw target fps line
        let line_color = Color::WHITE.multiply_alpha(ALPHA);
        let line_top = RESOLUTION_F.y - (RECT_HEIGHT as f32) - 1.0;

        rectangle.set_position(Vec2::new(0.0, line_top));
        rectangle.set_scale(Vec2::new(RESOLUTION_F.x, 1.0));
        rectangle.set_color(line_color);
        sprite_queue.draw_sprite(&rectangle);

        // draw network details
        let network_details = globals.network.details();
        let mut text = Text::new(game_io, FontName::ThinSmall);

        let mut whitespace_size = text.style.measure(" ").size;
        whitespace_size.x += text.style.letter_spacing;
        text.style.bounds.y = line_top - whitespace_size.y;

        // draw bytes up
        text.text = fmt_byte_avg(network_details.avg_bytes_up(), "/s UP ");
        text.style.bounds.x = RESOLUTION_F.x - text.measure().size.x;
        text.style.color = Color::from((200, 50, 200)).multiply_alpha(ALPHA); // purple
        text.draw(game_io, &mut sprite_queue);

        // draw bytes down
        text.text = fmt_byte_avg(network_details.avg_bytes_down(), "/s DOWN ");
        text.style.bounds.x = RESOLUTION_F.x - text.measure().size.x - whitespace_size.x * 14.0;
        text.style.color = Color::from((50, 200, 200)).multiply_alpha(ALPHA); // blue
        text.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn fmt_byte_avg(avg: usize, suffix: &str) -> String {
    let mut avg = avg as f32;

    if avg > 1024.0 {
        avg /= 1024.0;
    } else {
        return format!("{avg:.1} B{suffix}");
    }

    if avg > 1024.0 {
        avg /= 1024.0;
    } else {
        return format!("{avg:.1} KiB{suffix}");
    }

    format!("{avg:.1} MiB{suffix}")
}
