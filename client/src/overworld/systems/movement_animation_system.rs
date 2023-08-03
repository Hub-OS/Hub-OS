use crate::overworld::components::*;
use crate::overworld::OverworldArea;
use crate::render::AnimatorLoopMode;

pub fn system_movement_animation(area: &mut OverworldArea) {
    let entities = &mut area.entities;

    let query = entities.query_mut::<(&mut MovementAnimator, &mut Animator, &mut Direction)>();

    for (_, (movement_animator, animator, direction)) in query {
        if !movement_animator.animation_enabled() {
            continue;
        }

        let state = match find_state_from_movement(animator, movement_animator.state(), *direction)
        {
            Some(state) => state,
            None => continue,
        };

        if animator.current_state() != Some(state) {
            animator.set_state(state);
            animator.set_loop_mode(AnimatorLoopMode::Loop);
        }
    }
}

pub fn find_state_from_movement(
    animator: &Animator,
    movement_state: MovementState,
    direction: Direction,
) -> Option<&'static str> {
    let direction_tests = [direction, direction.rotate_c(), direction.rotate_cc()];

    for direction in direction_tests {
        if movement_state == MovementState::Running {
            let state = run_str(direction);

            if animator.has_state(state) {
                return Some(state);
            }
        }

        if movement_state == MovementState::Walking {
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
