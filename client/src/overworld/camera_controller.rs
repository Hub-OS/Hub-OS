use super::Map;
use crate::render::{Camera, CameraMotion};
use framework::prelude::*;
use std::collections::VecDeque;

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

pub struct CameraController {
    locked: bool,
    queue: VecDeque<CameraAction>,
    remaining_time: f32,
    player_entity: hecs::Entity,
    tracked_entity: Option<hecs::Entity>,
}

impl CameraController {
    pub fn new(player_entity: hecs::Entity) -> Self {
        Self {
            locked: false,
            queue: VecDeque::new(),
            remaining_time: 0.0,
            player_entity,
            tracked_entity: None,
        }
    }

    pub fn queue_action(&mut self, action: CameraAction) {
        self.locked |= action.locks_camera();
        self.queue.push_back(action);
    }

    pub fn is_locked(&self) -> bool {
        self.locked
    }

    pub fn update(
        &mut self,
        game_io: &GameIO,
        map: &Map,
        entities: &mut hecs::World,
        camera: &mut Camera,
    ) {
        if !self.locked {
            let entity = self.tracked_entity.unwrap_or(self.player_entity);

            if let Ok(target) = entities.query_one_mut::<&Vec3>(entity) {
                let target = map.world_3d_to_screen(*target).floor();
                camera.snap(target);
            }
        }

        camera.update(game_io);

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
                CameraAction::Shake { strength, duration } => camera.shake(strength, duration),
                CameraAction::Fade { color, duration } => camera.fade(color, duration),
                CameraAction::TrackEntity { entity } => self.tracked_entity = Some(entity),
                CameraAction::Unlock => {
                    // unlock as long as nothing else is queued
                    self.locked = !self.queue.is_empty();
                }
            }
        }
    }
}
