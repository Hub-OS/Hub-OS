use crate::overworld::components::*;
use crate::render::AnimatorLoopMode;

pub fn system_movement_animation(entities: &mut hecs::World) {
    let query = entities.query_mut::<(&mut MovementAnimator, &mut Animator, &mut Direction)>();

    for (_, (movement_animator, animator, direction)) in query {
        movement_animator.set_latest_direction(*direction);

        if !movement_animator.animation_enabled() {
            continue;
        }

        let Some(state) = find_state_from_movement(
            movement_animator,
            animator,
            movement_animator.state(),
            *direction,
        ) else {
            continue;
        };

        if animator.current_state() != Some(state) {
            animator.set_state(state);
            animator.set_loop_mode(AnimatorLoopMode::Loop);
        }
    }
}

pub fn find_state_from_movement(
    movement_animator: &MovementAnimator,
    animator: &Animator,
    movement_state: MovementState,
    direction: Direction,
) -> Option<&'static str> {
    // hand picked fallback directions
    let direction_tests = match direction {
        Direction::Left => [direction, Direction::DownLeft, Direction::UpLeft],
        Direction::Right => [direction, Direction::DownRight, Direction::UpRight],
        Direction::Up => {
            let last_horizontal = movement_animator.last_horizontal();

            [
                direction,
                last_horizontal.join(Direction::Up),
                last_horizontal.horizontal_mirror().join(Direction::Up),
            ]
        }
        Direction::Down => {
            let last_horizontal = movement_animator.last_horizontal();

            [
                direction,
                last_horizontal.horizontal_mirror().join(Direction::Down),
                last_horizontal.join(Direction::Down),
            ]
        }
        Direction::UpLeft => [direction, Direction::Left, Direction::Up],
        Direction::UpRight => [direction, Direction::Right, Direction::Up],
        Direction::DownLeft => [direction, Direction::Left, Direction::Down],
        Direction::DownRight => [direction, Direction::Right, Direction::Down],
        Direction::None => [Direction::Down, Direction::DownLeft, Direction::DownRight],
    };

    for direction in direction_tests {
        if movement_state == MovementState::Running {
            let state = run_str(direction);

            if animator.has_state(state) {
                return Some(state);
            }
        }

        if matches!(
            movement_state,
            MovementState::Walking | MovementState::Running
        ) {
            let state = walk_str(direction);

            if animator.has_state(state) {
                return Some(state);
            }
        }

        let state = idle_str(direction);

        if animator.has_state(state) {
            return Some(state);
        }
    }

    None
}

pub fn idle_str(direction: Direction) -> &'static str {
    match direction {
        Direction::Up => "IDLE_U",
        Direction::UpRight => "IDLE_UR",
        Direction::Right => "IDLE_R",
        Direction::DownRight => "IDLE_DR",
        Direction::Down => "IDLE_D",
        Direction::DownLeft => "IDLE_DL",
        Direction::Left => "IDLE_L",
        Direction::UpLeft => "IDLE_UL",
        Direction::None => "",
    }
}

pub fn walk_str(direction: Direction) -> &'static str {
    match direction {
        Direction::Up => "WALK_U",
        Direction::UpRight => "WALK_UR",
        Direction::Right => "WALK_R",
        Direction::DownRight => "WALK_DR",
        Direction::Down => "WALK_D",
        Direction::DownLeft => "WALK_DL",
        Direction::Left => "WALK_L",
        Direction::UpLeft => "WALK_UL",
        Direction::None => "",
    }
}

pub fn run_str(direction: Direction) -> &'static str {
    match direction {
        Direction::Up => "RUN_U",
        Direction::UpRight => "RUN_UR",
        Direction::Right => "RUN_R",
        Direction::DownRight => "RUN_DR",
        Direction::Down => "RUN_D",
        Direction::DownLeft => "RUN_DL",
        Direction::Left => "RUN_L",
        Direction::UpLeft => "RUN_UL",
        Direction::None => "",
    }
}
