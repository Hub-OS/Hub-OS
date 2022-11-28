use crate::overworld::components::*;
use crate::resources::*;
use crate::scenes::OverworldSceneBase;
use framework::prelude::*;

pub fn system_player_movement(game_io: &mut GameIO<Globals>, scene: &mut OverworldSceneBase) {
    let input_util = InputUtil::new(game_io);

    let input_direction = if scene.is_input_locked(game_io) {
        Direction::None
    } else {
        input_util.direction()
    };

    let player_data = &scene.player_data;
    let entities = &mut scene.entities;

    let movement_animator = entities
        .query_one_mut::<&mut MovementAnimator>(player_data.entity)
        .unwrap();

    if input_direction == Direction::None {
        // a little inaccurate, immediate stop, allows for better diagonal stops on keyboard
        movement_animator.set_state(MovementState::Idle);
    } else if input_util.is_down(Input::Sprint) {
        movement_animator.set_state(MovementState::Running);
    } else {
        movement_animator.set_state(MovementState::Walking);
    }

    movement_animator.queue_direction(input_direction);
}
