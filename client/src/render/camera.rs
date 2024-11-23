use super::FrameTime;
use crate::resources::*;
use framework::prelude::*;
use rand::Rng;

trait IsCloseEnough {
    fn is_close_enough(&self, other: &Self) -> bool;
}

impl IsCloseEnough for f32 {
    fn is_close_enough(&self, other: &Self) -> bool {
        (*other - *self).abs() < 1.0
    }
}

impl IsCloseEnough for Vec2 {
    fn is_close_enough(&self, other: &Self) -> bool {
        (*other - *self).max_element() < 1.0
    }
}

#[derive(Clone, Copy)]
pub enum CameraMotion {
    Lerp { duration: FrameTime },
    Wane { duration: FrameTime, factor: f32 },
    Multiply { factor: f32 },
    None,
}

impl CameraMotion {
    fn interpolate<T>(&self, start: T, end: T, elapsed: FrameTime) -> (T, bool)
    where
        T: IsCloseEnough
            + std::ops::Mul<f32, Output = T>
            + std::ops::Sub<Output = T>
            + std::ops::Add<Output = T>
            + Copy,
    {
        match *self {
            CameraMotion::Lerp { duration } => {
                if elapsed > duration {
                    return (end, true);
                }

                let progress = elapsed as f32 / duration as f32;

                ((end - start) * progress + start, false)
            }
            CameraMotion::Wane { duration, factor } => {
                if elapsed > duration {
                    return (end, true);
                }

                let progress = elapsed as f32 / duration as f32;

                let x = wane(progress, factor);

                ((end - start) * x + start, false)
            }
            CameraMotion::Multiply { factor } => {
                let value = (end - start) * factor + start;

                (value, value.is_close_enough(&end))
            }
            CameraMotion::None => (end, true),
        }
    }
}

pub struct Camera {
    internal_camera: OrthoCamera,
    origin: Vec2,
    destination: Vec2,
    motion: CameraMotion,
    slide_elapsed: FrameTime,
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
        internal_camera.set_inverted_y(true);

        Self {
            internal_camera,
            origin: Vec2::ZERO,
            destination: Vec2::ZERO,
            slide_elapsed: 0,
            motion: CameraMotion::None,
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

    pub fn scale(&self) -> Vec2 {
        self.internal_camera.scale()
    }

    pub fn set_scale(&mut self, scale: Vec2) {
        self.internal_camera.set_scale(scale);
    }

    pub fn update(&mut self, game_io: &GameIO) {
        let last_frame_secs = (game_io.frame_duration() + game_io.sleep_duration()).as_secs_f32();
        self.slide_elapsed += 1;
        self.shake_progress += last_frame_secs;
        self.fade_progress += last_frame_secs;

        let x = (self.fade_progress / self.fade_duration).min(1.0);

        self.current_color = Color::lerp(self.start_color, self.fade_color, x);

        let (position, slide_completed) =
            self.motion
                .interpolate(self.origin, self.destination, self.slide_elapsed);

        if slide_completed {
            self.motion = CameraMotion::None;
        }

        self.internal_camera
            .set_position(position.floor().extend(0.0));

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

    pub fn slide(&mut self, destination: Vec2, motion: CameraMotion) {
        self.origin = Vec2::new(self.internal_camera.x(), self.internal_camera.y());
        self.slide_elapsed = 0;
        self.destination = destination;
        self.motion = motion;
    }

    pub fn snap(&mut self, mut destination: Vec2) {
        destination = destination.floor();

        self.internal_camera.set_position(destination.extend(0.0));

        self.slide(destination, CameraMotion::None);
    }

    fn offset(&mut self, offset: Vec2) {
        let position = self.internal_camera.position() + offset.extend(0.0);
        self.internal_camera.set_position(position.floor());
    }

    pub fn shake(&mut self, stress: f32, duration: f32) {
        if self.shaking && duration < self.shake_duration - self.shake_progress {
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
        internal_camera.set_inverted_y(true);

        Self {
            internal_camera,
            origin: self.origin,
            destination: self.destination,
            slide_elapsed: self.slide_elapsed,
            motion: CameraMotion::None,
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

fn wane(progress: f32, factor: f32) -> f32 {
    let result = (2.0 + (factor * progress.ln())) * 0.5;
    result.min(1.0)
}
