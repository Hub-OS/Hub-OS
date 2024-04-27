use packets::structures::Direction;
use std::collections::VecDeque;

#[derive(Debug, Default, Clone, Copy, PartialEq, Eq)]
pub enum MovementState {
    #[default]
    Idle,
    Walking,
    Running,
}

pub struct MovementAnimator {
    animation_enabled: bool,
    movement_enabled: bool,
    state: MovementState,
    direction_queue: VecDeque<Direction>,
    last_horizontal: Direction,
}

impl MovementAnimator {
    pub fn new() -> Self {
        let mut direction_queue = VecDeque::new();
        // 3 frame buffer, just as it appears to exist in the games
        direction_queue.extend([Direction::None, Direction::None, Direction::None]);

        Self {
            animation_enabled: true,
            movement_enabled: false,
            state: MovementState::Idle,
            direction_queue,
            last_horizontal: Direction::Right,
        }
    }

    pub fn animation_enabled(&self) -> bool {
        self.animation_enabled
    }

    pub fn set_animation_enabled(&mut self, enabled: bool) {
        self.animation_enabled = enabled;
    }

    pub fn movement_enabled(&self) -> bool {
        self.movement_enabled
    }

    pub fn set_movement_enabled(&mut self, enabled: bool) {
        self.movement_enabled = enabled;
    }

    pub fn state(&self) -> MovementState {
        self.state
    }

    pub fn set_state(&mut self, state: MovementState) {
        self.state = state;
    }

    pub fn fill_direction_queue(&mut self, direction: Direction) {
        for dir in &mut self.direction_queue {
            *dir = direction;
        }
    }

    pub fn clear_direction_queue(&mut self) {
        self.fill_direction_queue(Direction::None);
    }

    pub fn queue_direction(&mut self, direction: Direction) {
        if let Some(value) = self.direction_queue.back_mut() {
            *value = direction;
        }
    }

    pub fn advance_direction(&mut self) -> Direction {
        self.direction_queue.push_back(Direction::None);
        self.direction_queue.pop_front().unwrap()
    }

    pub fn set_latest_direction(&mut self, direction: Direction) {
        let horizontal = direction.split().0;

        if !horizontal.is_none() {
            self.last_horizontal = horizontal;
        }
    }

    pub fn last_horizontal(&self) -> Direction {
        self.last_horizontal
    }
}
