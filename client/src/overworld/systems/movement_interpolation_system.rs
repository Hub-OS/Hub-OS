use crate::overworld::OverworldArea;
use crate::overworld::components::{ActorPropertyAnimator, MovementAnimator, MovementInterpolator};
use framework::prelude::*;
use packets::structures::Direction;

pub fn system_movement_interpolation(game_io: &GameIO, area: &mut OverworldArea) {
    let entities = &mut area.entities;

    type Query<'a> = hecs::Without<
        (
            &'a mut Vec3,
            &'a mut Direction,
            &'a mut MovementInterpolator,
            &'a mut MovementAnimator,
        ),
        &'a ActorPropertyAnimator,
    >;

    for (_, (position, direction, interpolator, movement_animator)) in entities.query_mut::<Query>()
    {
        let (new_position, new_direction, new_state) = interpolator.update(game_io);

        if *position != new_position {
            *position = new_position;
            movement_animator.set_animation_enabled(true);
        }

        if !new_direction.is_none() {
            *direction = new_direction;
        }

        movement_animator.set_state(new_state);
    }
}
