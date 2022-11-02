use crate::overworld::components::{MovementAnimator, MovementInterpolator};
use crate::resources::Globals;
use crate::scenes::OverworldSceneBase;
use framework::prelude::*;
use packets::structures::Direction;

pub fn movement_interpolation_system(game_io: &GameIO<Globals>, scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;

    // todo: disable if property animator is animating position?

    for (_, (position, direction, interpolater, movement_animator)) in entities.query_mut::<(
        &mut Vec3,
        &mut Direction,
        &mut MovementInterpolator,
        &mut MovementAnimator,
    )>() {
        let (new_position, new_direction, new_state) = interpolater.update(game_io);
        *position = new_position;
        *direction = new_direction;
        movement_animator.state = new_state;
    }
}
