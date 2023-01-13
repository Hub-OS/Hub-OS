use crate::resources::*;
use framework::prelude::*;
use rand::Rng;

pub struct Camera {
    internal_camera: OrthoCamera,
    origin: Vec2,
    destination: Vec2,
    slide_progress: f32,
    slide_duration: f32,
    waning: bool,
    wane_factor: f32,
    shaking: bool,
    shake_stress: f32,
    shake_progress: f32,
    shake_duration: f32,
    start_color: Color,
    fade_color: Color,
    fade_progress: f32,
    fade_duration: f32,
    current_color: Color,
}

impl Camera {
    pub fn new(game_io: &GameIO) -> Self {
        let mut internal_camera = OrthoCamera::new(game_io, RESOLUTION_F);
        internal_camera.invert_y(true);

        Self {
            internal_camera,
            origin: Vec2::ZERO,
            destination: Vec2::ZERO,
            slide_progress: 0.0,
            slide_duration: 0.0,
            wane_factor: 0.0,
            waning: false,
            shaking: false,
            shake_stress: 0.0,
            shake_progress: 0.0,
            shake_duration: 0.0,
            start_color: Color::TRANSPARENT,
            fade_color: Color::TRANSPARENT,
            fade_progress: 0.0,
            fade_duration: 0.0,
            current_color: Color::TRANSPARENT,
        }
    }

    pub fn new_ui(game_io: &GameIO) -> Self {
        let mut camera = Self::new(game_io);

        camera.snap(RESOLUTION_F * 0.5);

        camera
    }

    pub fn position(&self) -> Vec2 {
        self.internal_camera.position().xy()
    }

    pub fn size(&self) -> Vec2 {
        self.internal_camera.size()
    }

    pub fn bounds(&self) -> Rect {
        self.internal_camera.bounds()
    }

    pub fn update(&mut self, game_io: &GameIO) {
        let last_frame_secs = (game_io.frame_duration() + game_io.sleep_duration()).as_secs_f32();
        self.slide_progress += last_frame_secs;
        self.shake_progress += last_frame_secs;
        self.fade_progress += last_frame_secs;

        let x = (self.fade_progress / self.fade_duration).min(1.0);

        self.current_color = Color::lerp(self.start_color, self.fade_color, x);

        // If progress is over, update position to the destination
        if self.slide_progress >= self.slide_duration {
            self.internal_camera
                .set_position(self.destination.extend(0.0));

            // reset wane params
            self.waning = false;
        } else {
            // Otherwise calculate the delta
            let delta = if self.waning {
                let x = wane(self.slide_progress, self.slide_duration, self.wane_factor);

                (self.destination - self.origin) * x + self.origin
            } else {
                let progress_percent = self.slide_progress / self.slide_duration;

                (self.destination - self.origin) * progress_percent + self.origin
            };

            self.internal_camera.set_position(delta.extend(0.0));
        }

        if self.shaking {
            if self.shake_progress <= self.shake_duration {
                // Drop off to zero by end of shake
                let curr_stress =
                    self.shake_stress * (1.0 - (self.shake_progress / self.shake_duration));

                let angle = rand::thread_rng().gen::<f32>() * std::f32::consts::TAU;

                let offset = Vec2::new(angle.sin() * curr_stress, angle.cos() * curr_stress);

                self.offset(offset);
            } else {
                self.shaking = false;
            }
        }
    }

    pub fn slide(&mut self, destination: Vec2, duration: f32) {
        self.origin = Vec2::new(self.internal_camera.x(), self.internal_camera.y());
        self.slide_progress = 0.0;
        self.destination = destination;
        self.slide_duration = duration;
        self.waning = false;
    }

    pub fn wane(&mut self, destination: Vec2, duration: f32, factor: f32) {
        self.slide(destination, duration);
        self.waning = true;
        self.wane_factor = factor;
    }

    pub fn snap(&mut self, destination: Vec2) {
        self.internal_camera.set_position(destination.extend(0.0));

        self.slide(destination, 0.0);
    }

    fn offset(&mut self, offset: Vec2) {
        let position = self.internal_camera.position() + offset.extend(0.0);
        self.internal_camera.set_position(position);
    }

    pub fn shake(&mut self, stress: f32, duration: f32) {
        if self.shaking {
            return;
        }

        self.shaking = true;
        self.shake_stress = stress;
        self.shake_duration = duration;
        self.shake_progress = 0.0;
    }

    pub fn fade(&mut self, color: Color, duration: f32) {
        self.start_color = self.current_color;
        self.fade_progress = 0.0;
        self.fade_color = color;
        self.fade_duration = duration;
    }

    pub fn lens_tint(&self) -> Color {
        self.current_color
    }

    pub fn clone(&self, game_io: &GameIO) -> Self {
        let mut internal_camera = OrthoCamera::new(game_io, RESOLUTION_F);
        internal_camera.invert_y(true);

        Self {
            internal_camera,
            origin: self.origin,
            destination: self.destination,
            slide_progress: self.slide_progress,
            slide_duration: self.slide_duration,
            waning: self.waning,
            wane_factor: self.wane_factor,
            shaking: self.shaking,
            shake_stress: self.shake_stress,
            shake_progress: self.shake_progress,
            shake_duration: self.shake_duration,
            start_color: self.start_color,
            fade_color: self.fade_color,
            fade_progress: self.fade_progress,
            fade_duration: self.fade_duration,
            current_color: self.current_color,
        }
    }
}

impl AsBinding for Camera {
    fn as_binding(&self) -> BindingResource<'_> {
        self.internal_camera.as_binding()
    }
}

fn wane(x: f32, length: f32, factor: f32) -> f32 {
    let result = (2.0 + (factor * (x / length).ln())) * 0.5;
    result.min(1.0)
}
