use super::FrameTime;
use crate::resources::*;
use framework::prelude::*;

#[derive(Default, Clone, Copy, Debug)]
pub enum CameraMotion {
    Lerp {
        duration: FrameTime,
    },
    Wane {
        duration: FrameTime,
        factor: f32,
    },
    Multiply {
        factor: f32,
    },
    #[default]
    None,
}

impl CameraMotion {
    fn interpolate<T>(
        &self,
        start: T,
        end: T,
        current: T,
        elapsed: FrameTime,
        is_close_enough: impl Fn(T, T) -> bool,
    ) -> (T, bool)
    where
        T: std::ops::Mul<f32, Output = T>
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
                let value = (end - current) * factor + current;

                if is_close_enough(value, end) {
                    return (end, true);
                }

                (value, false)
            }
            CameraMotion::None => (end, true),
        }
    }
}

#[derive(Clone, Default)]
struct CameraInterpolator<T> {
    start: T,
    end: T,
    current: T,
    elapsed: FrameTime,
    motion: CameraMotion,
}

impl<T> CameraInterpolator<T>
where
    T: std::ops::Mul<f32, Output = T>
        + std::ops::Sub<Output = T>
        + std::ops::Add<Output = T>
        + Copy,
{
    pub fn new(start: T) -> Self {
        Self {
            start,
            end: start,
            current: start,
            elapsed: 0,
            motion: CameraMotion::None,
        }
    }

    pub fn new_motion(start: T, end: T, motion: CameraMotion) -> Self {
        Self {
            start,
            end,
            current: start,
            elapsed: 0,
            motion,
        }
    }

    pub fn step(&mut self, is_close_enough: impl Fn(T, T) -> bool) -> T {
        self.elapsed += 1;
        let (value, completed) = self.motion.interpolate(
            self.start,
            self.end,
            self.current,
            self.elapsed,
            is_close_enough,
        );

        self.current = value;

        if completed {
            self.motion = CameraMotion::None;
        }

        value
    }
}

pub struct Camera {
    internal_camera: OrthoCamera,
    slide_interpolator: CameraInterpolator<Vec2>,
    zoom_interpolator: CameraInterpolator<Vec2>,
    shake_stress: f32,
    shake_elapsed: FrameTime,
    shake_duration: FrameTime,
    start_color: Color,
    fade_color: Color,
    fade_elapsed: FrameTime,
    fade_duration: FrameTime,
    current_color: Color,
}

impl Camera {
    pub fn new(game_io: &GameIO) -> Self {
        let mut internal_camera = OrthoCamera::new(game_io, RESOLUTION_F);
        internal_camera.set_inverted_y(true);

        Self {
            internal_camera,
            slide_interpolator: CameraInterpolator::default(),
            zoom_interpolator: CameraInterpolator::new(Vec2::ONE),
            shake_stress: 0.0,
            shake_elapsed: 0,
            shake_duration: 0,
            start_color: Color::TRANSPARENT,
            fade_color: Color::TRANSPARENT,
            fade_elapsed: 0,
            fade_duration: 0,
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
        self.zoom_interpolator = CameraInterpolator::new(scale);
    }

    pub fn update(&mut self) {
        self.shake_elapsed += 1;
        self.fade_elapsed += 1;

        let x = (self.fade_elapsed as f32 / self.fade_duration as f32).min(1.0);

        self.current_color = Color::lerp(self.start_color, self.fade_color, x);

        let position = self.slide_interpolator.step(|a, b| a.abs_diff_eq(b, 0.99));
        self.internal_camera
            .set_position(position.floor().extend(0.0));

        let scale = self.zoom_interpolator.step(|a, b| a.abs_diff_eq(b, 0.01));
        self.internal_camera.set_scale(scale);

        #[cfg(not(feature = "record_every_frame"))]
        if self.shake_elapsed <= self.shake_duration {
            use rand::Rng;

            let progress = self.shake_elapsed as f32 / self.shake_duration as f32;

            // Drop off to zero by end of shake
            let curr_stress = self.shake_stress * (1.0 - progress);

            let angle = rand::rng().random::<f32>() * std::f32::consts::TAU;

            let offset = Vec2::new(angle.sin() * curr_stress, angle.cos() * curr_stress);

            self.offset(offset);
        }
    }

    pub fn zoom(&mut self, scale: Vec2, motion: CameraMotion) {
        let start = self.zoom_interpolator.current;
        self.zoom_interpolator = CameraInterpolator::new_motion(start, scale, motion);
    }

    pub fn slide(&mut self, destination: Vec2, motion: CameraMotion) {
        let start = self.slide_interpolator.current;
        self.slide_interpolator = CameraInterpolator::new_motion(start, destination, motion);
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

    pub fn shake(&mut self, stress: f32, duration: FrameTime) {
        let shaking = self.shake_elapsed < self.shake_duration;

        if shaking && duration < self.shake_duration - self.shake_elapsed {
            return;
        }

        self.shake_stress = stress;
        self.shake_duration = duration;
        self.shake_elapsed = 0;
    }

    pub fn fade(&mut self, color: Color, duration: FrameTime) {
        self.start_color = self.current_color;
        self.fade_elapsed = 0;
        self.fade_color = color;
        self.fade_duration = duration;
    }

    pub fn lens_tint(&self) -> Color {
        self.current_color
    }

    pub fn clone(&self, game_io: &GameIO) -> Self {
        let mut internal_camera = OrthoCamera::new(game_io, RESOLUTION_F);
        internal_camera.set_inverted_y(true);
        internal_camera.set_position(self.position().extend(0.0));
        internal_camera.set_scale(self.scale());

        Self {
            internal_camera,
            slide_interpolator: self.slide_interpolator.clone(),
            zoom_interpolator: self.zoom_interpolator.clone(),
            shake_stress: self.shake_stress,
            shake_elapsed: self.shake_elapsed,
            shake_duration: self.shake_duration,
            start_color: self.start_color,
            fade_color: self.fade_color,
            fade_elapsed: self.fade_elapsed,
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
