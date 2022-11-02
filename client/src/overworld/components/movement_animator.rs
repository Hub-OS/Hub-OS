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
    pub enabled: bool,
    pub state: MovementState,
    direction_queue: VecDeque<Direction>,
}

impl MovementAnimator {
    pub fn new() -> Self {
        let mut direction_queue = VecDeque::new();
        // 3 frame buffer, just as it appears to exist in the games
        direction_queue.extend([Direction::None, Direction::None, Direction::None]);

        Self {
            enabled: true,
            state: MovementState::Idle,
            direction_queue,
        }
    }

    pub fn queue_direction(&mut self, direction: Direction) -> Direction {
        self.direction_queue.push_back(direction);
        self.direction_queue.pop_front().unwrap()
    }
}
