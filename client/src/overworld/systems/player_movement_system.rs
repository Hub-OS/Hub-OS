pub const WALK_SPEED: f32 = 80.0 / 60.0;
pub const RUN_SPEED: f32 = 140.0 / 60.0;
const COLLISION_RADIUS: f32 = 4.0;

use crate::overworld::components::*;
use crate::overworld::*;
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
    let map = &scene.map;

    let (position, direction, movement_animator) = entities
        .query_one_mut::<(&mut Vec3, &mut Direction, &mut MovementAnimator)>(player_data.entity)
        .unwrap();

    let original_position = *position;

    let movement_direction = movement_animator.queue_direction(input_direction);

    if input_direction == Direction::None {
        // a little inaccurate, immediate stop, allows for better diagonal stops on keyboard
        movement_animator.state = MovementState::Idle;
        return;
    }

    *direction = movement_direction;

    // update position
    let mut speed;

    if input_util.is_down(Input::Sprint) {
        movement_animator.state = MovementState::Running;
        speed = RUN_SPEED;
    } else {
        movement_animator.state = MovementState::Walking;
        speed = WALK_SPEED;
    };

    if position.z.fract() != 0.0 {
        speed = RUN_SPEED;
    }

    let final_position = find_final_position(
        player_data,
        entities,
        map,
        original_position,
        movement_direction,
        speed,
    );

    let player_position = entities
        .query_one_mut::<&mut Vec3>(player_data.entity)
        .unwrap();

    *player_position = final_position;
}

fn find_final_position(
    player_state: &OverworldPlayerData,
    entities: &mut hecs::World,
    map: &Map,
    original_position: Vec3,
    direction: Direction,
    speed: f32,
) -> Vec3 {
    // test alternate directions, this provides a sliding effect
    let offset = offset_with_direction(map, direction, speed);

    let (first_test, first_position) = try_move_to(
        player_state,
        entities,
        map,
        original_position,
        offset,
        speed,
    );

    if first_test {
        return first_position;
    }

    let offset = offset_with_direction(map, direction.rotate_c(), speed);

    let (second_test, second_position) = try_move_to(
        player_state,
        entities,
        map,
        original_position,
        offset,
        speed,
    );

    if second_test {
        return second_position;
    }

    let offset = offset_with_direction(map, direction.rotate_cc(), speed);

    let (third_test, third_position) = try_move_to(
        player_state,
        entities,
        map,
        original_position,
        offset,
        speed,
    );

    if third_test {
        return third_position;
    }

    first_position
}

fn offset_with_direction(map: &Map, direction: Direction, magnitude: f32) -> Vec2 {
    let mut offset: Vec2 = direction.chebyshev_vector().into();

    if direction.is_diagonal() {
        offset.y /= 2.0;
    }

    map.screen_to_world(offset * magnitude)
}

fn try_move_to(
    player_state: &OverworldPlayerData,
    entities: &mut hecs::World,
    map: &Map,
    current_pos: Vec3,
    offset: Vec2,
    speed: f32,
) -> (bool, Vec3) {
    let max_elevation_diff = (speed + 1.0) / map.tile_size().y as f32 * 2.0;

    let mut target_pos = current_pos + offset.extend(0.0);

    let curr_layer = current_pos.z as i32;
    let curr_pos_tile_space = map.world_to_tile_space(current_pos.xy());
    let target_pos_tile_space = map.world_to_tile_space(target_pos.xy());
    let tile_speed = curr_pos_tile_space.distance(target_pos_tile_space);

    let layer_relative_elevation = current_pos.z.fract();
    let new_layer = get_target_layer(
        map,
        curr_layer,
        layer_relative_elevation,
        tile_speed,
        curr_pos_tile_space,
        target_pos_tile_space,
    );

    target_pos.z = map.elevation_at(target_pos_tile_space, new_layer);

    let ray = offset / speed * COLLISION_RADIUS;

    /*
     * Test map collisions
     * This should be a cheaper check to do first
     * before checking neighboring actors
     */
    if new_layer >= 0 && map.tile_layers().len() as i32 > new_layer {
        if (target_pos.z - current_pos.z).abs() > max_elevation_diff {
            // height difference is too great
            return (false, current_pos);
        }

        // detect collision at the edge of the collisionRadius

        let edge_tile_space = map.world_to_tile_space(current_pos.xy() + ray);

        let edge_layer = get_target_layer(
            map,
            curr_layer,
            layer_relative_elevation,
            tile_speed,
            curr_pos_tile_space,
            edge_tile_space,
        );

        let edge_elevation = map.elevation_at(edge_tile_space, edge_layer);

        let can_move_to_edge = map.can_move_to(edge_tile_space.extend(edge_elevation));

        if !can_move_to_edge || (target_pos.z - edge_elevation).abs() > max_elevation_diff {
            return (false, current_pos);
        }
    }

    // if we can move forward, check neighboring actors
    for (entity, (other_pos, ActorCollider { radius, solid })) in
        entities.query_mut::<(&Vec3, &ActorCollider)>()
    {
        if !solid || entity == player_state.entity {
            continue;
        }

        let elevation_difference = (other_pos.z - target_pos.z).abs();

        if elevation_difference > 0.1 {
            continue;
        }

        let delta = target_pos - *other_pos;
        let collision = delta.length_squared() <= (COLLISION_RADIUS + *radius).powi(2);

        if collision {
            // push the ourselves out of the other actor
            // use current position to prevent sliding off map
            let delta_unit = delta.normalize();
            let sum_of_radii = COLLISION_RADIUS + *radius;
            let out_pos = *other_pos + delta_unit * sum_of_radii;

            let out_pos_in_tile_space = map.world_to_tile_space(out_pos.xy());
            let elevation = map.elevation_at(out_pos_in_tile_space, new_layer);

            return (false, Vec3::new(out_pos.x, out_pos.y, elevation));
        }
    }

    (true, target_pos)
}

fn get_target_layer(
    map: &Map,
    current_layer: i32,
    layer_relative_elevation: f32,
    tile_speed: f32,
    current_tile_pos: Vec2,
    target_tile_pos: Vec2,
) -> i32 {
    // test going down
    let layer_below = current_layer - 1;
    let floor_is_hole = map.ignore_tile_above_f32(target_tile_pos, layer_below);

    if floor_is_hole {
        return layer_below;
    }

    // test going up
    let elevation_padding = 1.0 / map.tile_size().y as f32 * 2.0;
    let max_elevation_diff = tile_speed + elevation_padding;
    let can_climb = layer_relative_elevation >= 1.0 - max_elevation_diff;

    if can_climb && !same_tile(current_tile_pos, target_tile_pos) {
        // if we're at the top of stairs, target the layer above
        return current_layer + 1;
    }

    current_layer
}

fn same_tile(a: Vec2, b: Vec2) -> bool {
    a.floor() == b.floor()
}
