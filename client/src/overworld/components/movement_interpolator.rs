use crate::overworld::Map;
use crate::resources::{OVERWORLD_RUN_SPEED, OVERWORLD_RUN_THRESHOLD};
use framework::prelude::*;
use packets::structures::Direction;
use packets::{MAX_IDLE_DURATION, SERVER_TICK_RATE_F};

use super::MovementState;

const MAX_IDLE_MOVEMENT_SQR: f32 = 1.0;
const MIN_TELEPORT_DIST: f32 = OVERWORLD_RUN_SPEED * 60.0 * 8.0; // 8x run speed
const MIN_TELEPORT_DIST_SQR: f32 = MIN_TELEPORT_DIST * MIN_TELEPORT_DIST;

pub struct MovementInterpolator {
    last_movement: Instant,
    current_position: Vec3,
    last_position: Vec3,
    target_position: Vec3,
    last_target_position: Vec3,
    idle_direction: Direction,
    resolved_direction: Direction,
    resolved_movement_state: MovementState,
}

impl MovementInterpolator {
    pub fn new(game_io: &GameIO, position: Vec3, direction: Direction) -> Self {
        Self {
            last_movement: game_io.frame_start_instant(),
            current_position: position,
            last_position: position,
            target_position: position,
            last_target_position: position,
            idle_direction: direction,
            resolved_direction: direction,
            resolved_movement_state: MovementState::Idle,
        }
    }

    pub fn is_movement_impossible(&self, map: &Map, position: Vec3) -> bool {
        let diff = position.xy() - self.current_position.xy();
        let screen_diff = map.world_to_screen(diff);
        let screen_dist = screen_diff.length_squared();

        screen_dist >= MIN_TELEPORT_DIST_SQR * SERVER_TICK_RATE_F * SERVER_TICK_RATE_F
    }

    pub fn push(&mut self, game_io: &GameIO, map: &Map, position: Vec3, direction: Direction) {
        // update idle direction
        self.idle_direction = direction;

        // update positions for interpolation
        self.last_target_position = self.target_position;
        self.last_position = self.current_position;
        self.target_position = position;

        // resolve direction and movement state, before updating positions
        let world_delta = position.xy() - self.last_target_position.xy();
        let screen_delta = map.world_to_screen(world_delta);
        let screen_dist_sqr = screen_delta.length_squared();

        if world_delta == Vec2::ZERO {
            return;
        }

        self.last_movement = game_io.frame_start_instant();

        if screen_dist_sqr <= (OVERWORLD_RUN_THRESHOLD * 60.0 * SERVER_TICK_RATE_F).powi(2) {
            self.resolved_direction = Direction::from_offset(world_delta.xy().into());
            self.resolved_movement_state = MovementState::Walking;
        } else {
            self.resolved_direction = Direction::from_offset(world_delta.xy().into());
            self.resolved_movement_state = MovementState::Running;
        }
    }

    pub fn force_position(&mut self, position: Vec3) {
        self.last_position = position;
        self.current_position = position;
        self.target_position = position;
        self.resolved_movement_state = MovementState::Idle;
        self.resolved_direction = self.idle_direction;
    }

    pub fn update(&mut self, game_io: &GameIO) -> (Vec3, Direction, MovementState) {
        if self.resolved_movement_state != MovementState::Idle {
            let elapsed =
                game_io.frame_start_instant() - self.last_movement + game_io.target_duration();
            let elapsed_secs = elapsed.as_secs_f32();
            let elapsed_ticks = elapsed_secs * 60.0;

            // (average or SERVER_TICK_RATE_F) * FPS * SERVER_TICK_RATE
            let progress = SERVER_TICK_RATE_F * 60.0 * SERVER_TICK_RATE_F * elapsed_ticks;

            if (progress >= 1.0 && self.last_target_position == self.target_position)
                || game_io.frame_start_instant() - self.last_movement >= MAX_IDLE_DURATION
            {
                // idle
                self.force_position(self.target_position);
            } else {
                // interpolate
                let world_delta = self.target_position - self.last_position;
                self.current_position = (self.last_position + world_delta * progress).round();
            }
        }

        (
            self.current_position,
            self.resolved_direction,
            self.resolved_movement_state,
        )
    }
}
