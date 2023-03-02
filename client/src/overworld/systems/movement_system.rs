use crate::overworld::{components::*, Map, TileClass};
use crate::resources::{OVERWORLD_RUN_SPEED, OVERWORLD_WALK_SPEED};
use crate::scenes::OverworldSceneBase;
use framework::prelude::{Vec2, Vec3Swizzles};

const COLLISION_RADIUS: f32 = 4.0;

pub fn system_movement(scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;
    let map = &mut scene.map;

    let mut query = entities.query::<(&mut MovementAnimator, &Vec3)>();
    let mut updates = Vec::new();

    for (actor_entity, (movement_animator, position)) in query.into_iter() {
        if !movement_animator.movement_enabled() {
            movement_animator.clear_queue();
            continue;
        }

        if movement_animator.state() != MovementState::Idle {
            // force animation on if we're moving through the movement animator
            movement_animator.set_animation_enabled(true);
        }

        let movement_direction = movement_animator.advance_direction();

        let speed = match movement_animator.state() {
            MovementState::Idle => 0.0,
            MovementState::Walking => {
                if position.z.fract() != 0.0 {
                    OVERWORLD_RUN_SPEED
                } else {
                    OVERWORLD_WALK_SPEED
                }
            }
            MovementState::Running => OVERWORLD_RUN_SPEED,
        };

        let original_position = *position;
        let final_position = find_final_position(
            actor_entity,
            entities,
            map,
            original_position,
            movement_direction,
            speed,
        );

        updates.push((actor_entity, final_position, movement_direction));
    }

    std::mem::drop(query);

    for (actor_entity, final_position, final_direction) in updates {
        let (position, direction) = entities
            .query_one_mut::<(&mut Vec3, &mut Direction)>(actor_entity)
            .unwrap();

        *position = final_position;

        if !final_direction.is_none() {
            *direction = final_direction;
        }
    }
}

fn find_final_position(
    actor_entity: hecs::Entity,
    entities: &hecs::World,
    map: &Map,
    original_position: Vec3,
    direction: Direction,
    speed: f32,
) -> Vec3 {
    // test alternate directions, this provides a sliding effect
    let offset = offset_with_direction(map, direction, speed);

    let (first_test, first_position) = try_move_to(
        actor_entity,
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
        actor_entity,
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
        actor_entity,
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
    offset *= magnitude;
    offset = offset.round();

    if direction.is_diagonal() {
        offset.y /= 2.0;
    }

    map.screen_to_world(offset)
}

fn try_move_to(
    actor_entity: hecs::Entity,
    entities: &hecs::World,
    map: &Map,
    current_pos: Vec3,
    offset: Vec2,
    speed: f32,
) -> (bool, Vec3) {
    let max_elevation_diff = (speed + 1.0) / map.tile_size().y as f32 * 2.0;

    let mut target_pos = current_pos + offset.extend(0.0);

    let curr_layer = current_pos.z as i32;
    let curr_tile_pos = map.world_to_tile_space(current_pos.xy());
    let target_tile_pos = map.world_to_tile_space(target_pos.xy());
    let tile_speed = curr_tile_pos.distance(target_tile_pos);

    let layer_relative_elevation = current_pos.z.fract();
    let new_layer = get_target_layer(
        map,
        curr_layer,
        layer_relative_elevation,
        tile_speed,
        curr_tile_pos,
        target_tile_pos,
    );

    target_pos.z = map.elevation_at(target_tile_pos, new_layer);

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
            curr_tile_pos,
            edge_tile_space,
        );

        let edge_elevation = map.elevation_at(edge_tile_space, edge_layer);

        let can_move_to_edge = map.can_move_to(edge_tile_space.extend(edge_elevation));

        if !can_move_to_edge || (target_pos.z - edge_elevation).abs() > max_elevation_diff {
            return (false, current_pos);
        }
    }

    // if we can move forward, check neighboring actors
    let mut query = entities.query::<(&Vec3, &ActorCollider)>();

    for (entity, (&other_pos, &ActorCollider { radius, solid })) in query.into_iter() {
        if !solid || entity == actor_entity {
            continue;
        }

        let elevation_difference = (other_pos.z - target_pos.z).abs();

        if elevation_difference > 0.1 {
            continue;
        }

        let delta = target_pos - other_pos;
        let collision = delta.length_squared() < (COLLISION_RADIUS + radius).powi(2);

        if collision {
            // push the ourselves out of the other actor
            // use current position to prevent sliding off map
            let delta_unit = delta.normalize();
            let sum_of_radii = COLLISION_RADIUS + radius;
            let out_pos = other_pos + delta_unit * sum_of_radii;

            let out_tile_pos = map.world_to_tile_space(out_pos.xy());
            let elevation = map.elevation_at(out_tile_pos, new_layer);

            return (false, Vec3::new(out_pos.x, out_pos.y, elevation));
        }
    }

    // corner squeeze check for conveyors
    if target_pos.z >= 0.0
        && target_tile_pos.x.floor() != curr_tile_pos.x.floor()
        && target_tile_pos.y.floor() != curr_tile_pos.y.floor()
    {
        // should only matter for the target layer
        if let Some(layer) = map.tile_layer(target_pos.z as usize) {
            let mut x_tile_position = curr_tile_pos;
            let mut y_tile_position = curr_tile_pos;

            x_tile_position.x += (target_tile_pos.x - curr_tile_pos.x).signum();
            y_tile_position.y += (target_tile_pos.y - curr_tile_pos.y).signum();

            let x_tile = layer.tile_at_f32(x_tile_position);
            let y_tile = layer.tile_at_f32(y_tile_position);

            let x_meta = map.tile_meta_for_tile(x_tile.gid);
            let y_meta = map.tile_meta_for_tile(y_tile.gid);

            if matches!(x_meta, Some(meta) if meta.tile_class == TileClass::Conveyor)
                && matches!(y_meta, Some(meta) if meta.tile_class == TileClass::Conveyor)
            {
                return (false, current_pos);
            }
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
