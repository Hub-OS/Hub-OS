use super::Map;
use crate::render::{Camera, CameraMotion};
use crate::resources::{Globals, InputUtil};
use framework::prelude::*;
use packets::FreeCamOptions;
use packets::structures::Input;
use std::collections::VecDeque;

#[derive(Debug)]
pub enum CameraAction {
    Snap {
        target: Vec2,
        hold_duration: f32,
    },
    Slide {
        target: Vec2,
        duration: f32,
    },
    Wane {
        target: Vec2,
        duration: f32,
        factor: f32,
    },
    Shake {
        strength: f32,
        duration: f32,
    },
    Fade {
        color: Color,
        duration: f32,
    },
    TrackEntity {
        entity: hecs::Entity,
    },
    EnableFreeCam {
        options: FreeCamOptions,
    },
    DisableFreeCam,
    Unlock,
}

impl CameraAction {
    pub fn locks_camera(&self) -> bool {
        matches!(
            self,
            Self::Snap { .. } | Self::Slide { .. } | Self::Wane { .. }
        )
    }

    pub fn block_duration(&self) -> f32 {
        match self {
            CameraAction::Snap { hold_duration, .. } => *hold_duration,
            CameraAction::Slide { duration, .. } => *duration,
            CameraAction::Wane { duration, .. } => *duration,
            _ => 0.0,
        }
    }
}

struct FreeCamConfig {
    speed: f32,
    fast_speed: f32,
    rounding_error: Vec2,
}

pub struct CameraController {
    locked: bool,
    queue: VecDeque<CameraAction>,
    remaining_time: f32,
    player_entity: hecs::Entity,
    tracked_entity: Option<hecs::Entity>,
    free_cam: Option<FreeCamConfig>,
}

impl CameraController {
    pub fn new(player_entity: hecs::Entity) -> Self {
        Self {
            locked: false,
            queue: VecDeque::new(),
            remaining_time: 0.0,
            player_entity,
            tracked_entity: None,
            free_cam: None,
        }
    }

    pub fn queue_action(&mut self, action: CameraAction) {
        self.locked |= action.locks_camera();
        self.queue.push_back(action);
    }

    pub fn is_locked(&self) -> bool {
        self.locked
    }

    pub fn movement_controls_camera(&self) -> bool {
        self.free_cam.is_some()
    }

    pub fn update(
        &mut self,
        game_io: &GameIO,
        map: &Map,
        entities: &mut hecs::World,
        camera: &mut Camera,
    ) {
        if !self.locked {
            if let Some(free_cam_config) = &mut self.free_cam {
                // free cam
                let input = InputUtil::new(game_io);

                let mut offset = Vec2::new(
                    input.as_axis(Input::Left, Input::Right),
                    input.as_axis(Input::Up, Input::Down),
                );

                if offset.x != 0.0 {
                    offset.y *= 0.5;
                }

                if input.is_down(Input::Sprint) {
                    offset *= free_cam_config.fast_speed
                } else {
                    offset *= free_cam_config.speed
                }

                // avoid camera drifting over time from rounding, noticeable when moving diagonally
                let mut new_position = camera.position() + offset;
                new_position += free_cam_config.rounding_error;

                free_cam_config.rounding_error = new_position;
                new_position = new_position.floor();
                free_cam_config.rounding_error -= new_position;

                // clamp
                let bounds = map.camera_bounds();
                new_position.x = new_position.x.clamp(bounds.left(), bounds.right());
                new_position.y = new_position.y.clamp(bounds.top(), bounds.bottom());
                camera.snap(new_position);
            } else {
                // follow an entity
                let entity = self.tracked_entity.unwrap_or(self.player_entity);

                if let Ok(target) = entities.query_one_mut::<&Vec3>(entity) {
                    let target = map.world_3d_to_screen(*target).floor();
                    camera.snap(target);
                }
            }
        }

        camera.update();

        let last_frame_secs = (game_io.frame_duration() + game_io.sleep_duration()).as_secs_f32();

        self.remaining_time -= last_frame_secs;

        if self.remaining_time > 0.0 {
            return;
        }

        self.remaining_time = 0.0;

        while self.remaining_time == 0.0 {
            let Some(action) = self.queue.pop_front() else {
                return;
            };

            self.remaining_time = action.block_duration();

            match action {
                CameraAction::Snap { target, .. } => camera.snap(target),
                CameraAction::Slide { target, duration } => camera.slide(
                    target,
                    CameraMotion::Lerp {
                        duration: (duration * 60.0) as _,
                    },
                ),
                CameraAction::Wane {
                    target,
                    duration,
                    factor,
                } => camera.slide(
                    target,
                    CameraMotion::Wane {
                        duration: (duration * 60.0) as _,
                        factor,
                    },
                ),
                CameraAction::Shake { strength, duration } => {
                    let globals = Globals::from_resources(game_io);

                    if globals.config.screen_shake {
                        camera.shake(strength, (duration * 60.0) as _)
                    }
                }
                CameraAction::Fade { color, duration } => {
                    camera.fade(color, (duration * 60.0) as _)
                }
                CameraAction::TrackEntity { entity } => {
                    if entity == self.player_entity {
                        self.tracked_entity = None;
                        self.locked = self.queue.iter().any(|action| action.locks_camera());
                    } else {
                        self.tracked_entity = Some(entity)
                    }
                }
                CameraAction::EnableFreeCam { options } => {
                    self.free_cam = Some(FreeCamConfig {
                        speed: options.speed.unwrap_or(4.0),
                        fast_speed: options.fast_speed.unwrap_or(8.0),
                        rounding_error: Default::default(),
                    });
                }
                CameraAction::DisableFreeCam => {
                    self.free_cam = None;
                }
                CameraAction::Unlock => {
                    // unlock as long as nothing else is queued
                    self.locked = self.queue.iter().any(|action| action.locks_camera());
                }
            }
        }
    }
}
