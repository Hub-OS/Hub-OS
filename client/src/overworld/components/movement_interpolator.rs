use crate::overworld::{RUN_SPEED, WALK_SPEED};
use crate::resources::Globals;
use framework::prelude::*;
use packets::structures::Direction;
use packets::SERVER_TICK_RATE;

use super::MovementState;

const SMOOTHING: f32 = 3.0;
const WINDOW_LEN: usize = 40;
const WINDOW_LEN_F: f32 = WINDOW_LEN as f32;
const K: f32 = SMOOTHING / (1.0 + WINDOW_LEN as f32);

const MAX_IDLE_MOVEMENT_SQR: f32 = 1.0;
const MIN_TELEPORT_DIST_SQR: f32 = RUN_SPEED * RUN_SPEED * 60.0 * 60.0;

// todo: move to packets/lib.rs? need to make sure the server uses it too
const MAX_IDLE_DURATION: Duration = Duration::from_secs(1);

pub struct MovementInterpolator {
    last_push: Instant,
    average: f32,
    current_position: Vec3,
    last_position: Vec3,
    target_position: Vec3,
    idle_direction: Direction,
}

impl MovementInterpolator {
    pub fn new(game_io: &GameIO<Globals>, position: Vec3, direction: Direction) -> Self {
        Self {
            last_push: game_io.frame_start_instant(),
            average: SERVER_TICK_RATE.as_secs_f32(),
            current_position: position,
            last_position: position,
            target_position: position,
            idle_direction: direction,
        }
    }

    pub fn is_movement_impossible(&self, position: Vec3) -> bool {
        position.distance_squared(self.current_position) >= MIN_TELEPORT_DIST_SQR * self.average
    }

    pub fn push(&mut self, game_io: &GameIO<Globals>, position: Vec3, direction: Direction) {
        // uses EMA - exponential moving average
        let delay_duration = game_io.frame_start_instant() - self.last_push;
        let delay = delay_duration.as_secs_f32();

        if delay_duration <= MAX_IDLE_DURATION {
            self.average = delay * K + self.average * (1.0 - K);
        }

        let likely_idle = self.current_position.distance_squared(position) <= MAX_IDLE_MOVEMENT_SQR;

        self.last_push = game_io.frame_start_instant();
        self.last_position = if likely_idle {
            position
        } else {
            self.current_position
        };
        self.target_position = position;
        self.idle_direction = direction;
    }

    pub fn force_position(&mut self, position: Vec3) {
        self.last_position = position;
        self.current_position = position;
        self.target_position = position;
    }

    pub fn update(&mut self, game_io: &GameIO<Globals>) -> (Vec3, Direction, MovementState) {
        let delta_instant = game_io.frame_start_instant() - self.last_push;
        let delta = self.target_position - self.last_position;

        let distance_sqr = delta.length_squared();
        let likely_idle =
            distance_sqr <= MAX_IDLE_MOVEMENT_SQR || delta_instant >= MAX_IDLE_DURATION;

        let alpha = delta_instant.as_secs_f32() / self.average;
        let position = self.last_position + delta * alpha;

        let direction;
        let movement_state;

        if likely_idle {
            self.force_position(self.target_position);
            direction = self.idle_direction;
            movement_state = MovementState::Idle;
        } else if distance_sqr <= WALK_SPEED * self.average {
            direction = Direction::from_offset(delta.xy().into());
            movement_state = MovementState::Walking;
        } else {
            direction = Direction::from_offset(delta.xy().into());
            movement_state = MovementState::Running;
        }

        self.current_position = position;

        (position, direction, movement_state)
    }
}
