use crate::overworld::components::{ActorPropertyAnimator, MovementAnimator, MovementInterpolator};
use crate::scenes::OverworldSceneBase;
use framework::prelude::*;
use packets::structures::Direction;

pub fn system_movement_interpolation(game_io: &GameIO, scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;

    type Query<'a> = hecs::Without<
        (
            &'a mut Vec3,
            &'a mut Direction,
            &'a mut MovementInterpolator,
            &'a mut MovementAnimator,
        ),
        &'a ActorPropertyAnimator,
    >;

    for (_, (position, direction, interpolater, movement_animator)) in entities.query_mut::<Query>()
    {
        let (new_position, new_direction, new_state) = interpolater.update(game_io);
        *position = new_position;

        if !new_direction.is_none() {
            *direction = new_direction;
        }

        movement_animator.set_state(new_state);
    }
}
