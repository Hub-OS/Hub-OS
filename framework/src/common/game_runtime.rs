use crate::prelude::*;
use crate::{cfg_sdl, cfg_winit};
use instant::Instant;
use std::cell::RefCell;

pub(crate) struct GameRuntime<Globals: 'static> {
    event_buffer: Vec<WindowEvent>,
    scene_manager: SceneManager<Globals>,
    frame_end: Instant,
    game_io: GameIO<Globals>,
    overlay: Option<Box<dyn SceneOverlay<Globals>>>,
}

impl<Globals> GameRuntime<Globals> {
    pub(crate) async fn new(
        window: Window,
        loop_params: WindowLoopParams<Globals>,
    ) -> anyhow::Result<Self> {
        let window_size = window.size();
        let graphics = GraphicsContext::new(&window, window_size.x, window_size.y).await?;

        let mut game_io = GameIO::new(window, graphics, loop_params.globals_constructor);
        game_io.set_target_fps(loop_params.target_fps);

        let initial_scene = (loop_params.scene_constructor)(&mut game_io);

        let overlay = loop_params
            .overlay_constructor
            .map(|constructor| constructor(&mut game_io));

        Ok(Self {
            event_buffer: Vec::new(),
            scene_manager: SceneManager::new(initial_scene),
            frame_end: Instant::now(),
            game_io,
            overlay,
        })
    }

    pub fn is_quitting(&self) -> bool {
        self.game_io.is_quitting()
    }

    cfg_sdl! {
        pub async fn sleep(&self) {
            let sleep_duration = self.game_io.attempted_sleep_duration();

            if !sleep_duration.is_zero() {
                sleep(sleep_duration).await;
            }
        }
    }

    cfg_winit! {
        pub fn target_sleep_instant(&self) -> Instant {
            self.frame_end + self.game_io.attempted_sleep_duration()
        }
    }

    pub fn push_event(&mut self, event: WindowEvent) {
        self.event_buffer.push(event)
    }

    pub fn tick(&mut self) {
        if self.frame_end.elapsed() < self.game_io.attempted_sleep_duration() {
            // running too fast skip tick (this issue should only occur on web)
            return;
        }

        let start_instant = Instant::now();
        let game_io = &mut self.game_io;
        game_io.set_frame_start_instant(start_instant);

        // update the previous timing with new info before updates start
        let lost_duration = start_instant - self.frame_end - game_io.attempted_sleep_duration();
        game_io.set_lost_duration(lost_duration);

        // queue a new task
        let mut events = Vec::new();
        std::mem::swap(&mut events, &mut self.event_buffer);

        // update
        game_io.handle_tasks();
        game_io.handle_events(events);

        self.scene_manager.update(game_io);

        if let Some(overlay) = self.overlay.as_mut() {
            overlay.update(game_io);
        }

        // kick off new tasks
        game_io.handle_tasks();

        let update_instant = Instant::now();

        // draw - completing render?
        let graphics = game_io.graphics();
        let draw_duration = if let Ok(frame) = graphics.surface().get_current_texture() {
            let texture = &frame.texture;
            let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
            let texture_size = graphics.surface_size();

            let mut render_target = RenderTarget::from_view(view, texture_size);
            render_target.set_clear_color(graphics.clear_color());

            // actual draw
            // -- "continuation" instant, as there seems to be a significant amount of time between the update instant
            // -- and this one, likely completing rendering
            let render_continuation_instant = Instant::now();
            let device = graphics.device();

            let encoder = RefCell::new(device.create_command_encoder(
                &wgpu::CommandEncoderDescriptor {
                    label: Some("window_command_encoder"),
                },
            ));

            let mut render_pass = RenderPass::new(&encoder, &render_target);

            // draw scene
            self.scene_manager.draw(game_io, &mut render_pass);

            // draw overlay
            if let Some(overlay) = self.overlay.as_mut() {
                overlay.draw(game_io, &mut render_pass);
            }

            render_pass.flush();

            let queue = game_io.graphics().queue();

            queue.submit([encoder.into_inner().finish()]);
            frame.present();

            Instant::now() - render_continuation_instant
        } else {
            Duration::ZERO
        };

        let end_instant = Instant::now();

        // tracking time spent
        game_io.set_update_duration(update_instant - start_instant);
        game_io.set_draw_duration(draw_duration);
        game_io.set_frame_duration(end_instant - start_instant);
        game_io.update_sleep_duration();

        self.frame_end = end_instant;
    }
}
