use super::find_state_from_movement;
use crate::overworld::{components::*, OverworldArea, TileClass};
use crate::resources::*;
use framework::prelude::*;
use packets::structures::{ActorKeyFrame, ActorProperty, Ease};

const DEFAULT_CONVEYOR_SPEED: f32 = 6.0;
const DEFAULT_ICE_SPEED: f32 = 6.0;
const DEFAULT_TREADMILL_SPEED: f32 = 1.875;

pub fn system_player_movement(
    game_io: &mut GameIO,
    area: &mut OverworldArea,
    assets: &impl AssetManager,
) {
    system_base(game_io, area);
    system_tile_effect(game_io, area, assets);
}

fn system_base(game_io: &mut GameIO, area: &mut OverworldArea) {
    let input_util = InputUtil::new(game_io);

    let input_direction = if area.is_input_locked(game_io) {
        Direction::None
    } else {
        input_util.direction()
    };

    let player_data = &area.player_data;
    let entities = &mut area.entities;

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

fn system_tile_effect(game_io: &mut GameIO, area: &mut OverworldArea, assets: &impl AssetManager) {
    let player_data = &area.player_data;
    let entities = &mut area.entities;

    if entities.satisfies::<&ActorPropertyAnimator>(player_data.entity) == Ok(true) {
        return;
    }

    let (animator, position, &direction) = entities
        .query_one_mut::<(&Animator, &mut Vec3, &Direction)>(player_data.entity)
        .unwrap();

    if position.z < 0.0 {
        return;
    }

    let map = &area.map;

    let Some(layer) = map.tile_layer(position.z as usize) else {
        return;
    };

    let tile_position = map.world_3d_to_tile_space(*position);
    let tile = layer.tile_at_f32(tile_position.xy());

    let Some(tile_meta) = map.tile_meta_for_tile(tile.gid) else {
        return;
    };

    match tile_meta.tile_class {
        TileClass::Conveyor => {
            let screen_direction = tile.get_direction(tile_meta);
            let sound_effect = tile_meta.custom_properties.get("sound effect");
            let direction = map.screen_direction_to_world(screen_direction);
            let mut speed = tile_meta.custom_properties.get_f32("speed");

            if speed == 0.0 {
                speed = DEFAULT_CONVEYOR_SPEED;
            }

            let start_tile_position = tile_position;

            // resolve end position
            let mut end_tile_position = start_tile_position;
            let mut x_axis = false;

            match direction {
                Direction::Left | Direction::Right => {
                    x_axis = true;
                    end_tile_position.x = end_tile_position.x.floor();
                }
                Direction::Up | Direction::Down => {
                    x_axis = false;
                    end_tile_position.y = end_tile_position.y.floor();
                }
                _ => {}
            }

            let walk_vector: Vec3 = Vec2::from(direction.unit_vector()).extend(0.0);
            end_tile_position += walk_vector;

            // slide to the middle of the next tile if it's a conveyor
            let end_tile = layer.tile_at_f32(end_tile_position.xy());
            let end_tile_meta = map.tile_meta_for_tile(end_tile.gid);
            let end_tile_is_conveyor =
                matches!(end_tile_meta, Some(meta) if meta.tile_class == TileClass::Conveyor);

            if end_tile_is_conveyor {
                end_tile_position += walk_vector;
            }

            // fix overshooting
            let mut end_position = map.tile_3d_to_world(end_tile_position);

            match screen_direction {
                Direction::UpLeft => {
                    end_position.x += map.tile_size().x as f32 * 0.5 - 1.0;
                }
                Direction::UpRight => {
                    end_position.y += map.tile_size().y as f32 - 1.0;
                }
                _ => {}
            }

            end_tile_position = map.world_3d_to_tile_space(end_position);

            // resolve duration
            let tile_distance = (end_tile_position - start_tile_position)
                .abs()
                .max_element();

            let duration = tile_distance / speed;

            // begin actor proeprty animation
            let mut property_animator = ActorPropertyAnimator::new();

            // begin with sfx + idle animation state to carry through the animation
            let mut first_steps = vec![
                (
                    ActorProperty::SoundEffectLoop(sound_effect.to_string()),
                    Ease::Floor,
                ),
                (ActorProperty::Direction(screen_direction), Ease::Floor),
            ];

            if let Some(animation_state) =
                find_state_from_movement(animator, MovementState::Idle, screen_direction)
            {
                first_steps.push((
                    ActorProperty::Animation(animation_state.to_string()),
                    Ease::Floor,
                ))
            }

            property_animator.add_key_frame(ActorKeyFrame {
                property_steps: first_steps,
                duration: 0.0,
            });

            // bring us to the final position
            property_animator.add_key_frame(ActorKeyFrame {
                property_steps: vec![(
                    if x_axis {
                        ActorProperty::X(end_position.x)
                    } else {
                        ActorProperty::Y(end_position.y)
                    },
                    Ease::Linear,
                )],
                duration,
            });

            if !end_tile_is_conveyor {
                // small wait duration at a rest position
                property_animator.add_key_frame(ActorKeyFrame {
                    property_steps: Vec::new(),
                    duration: 0.25,
                });
            }

            let _ = entities.insert_one(player_data.entity, property_animator);
            ActorPropertyAnimator::start(game_io, assets, entities, player_data.entity);
        }
        TileClass::Ice => {
            let direction = map.screen_direction_to_world(direction);
            let sound_effect = tile_meta.custom_properties.get("sound effect");
            let mut speed = tile_meta.custom_properties.get_f32("speed");

            if speed == 0.0 {
                speed = DEFAULT_ICE_SPEED;
            }

            // resolve slide points
            let is_not_ice = |tile_position: IVec2, _: Vec2, _: Vec2| {
                let tile = layer.tile_at(tile_position);
                let tile_meta = map.tile_meta_for_tile(tile.gid);

                !matches!(tile_meta, Some(meta) if meta.tile_class == TileClass::Ice)
            };

            let start_tile_position = tile_position.xy();
            let ray = Vec2::from(direction.unit_vector()).round();
            let edge_tile_position =
                cast_ray(start_tile_position, ray, is_not_ice) + map.world_to_tile_space(ray); // overshoot to get off the ice
            let edge_position = map.tile_to_world(edge_tile_position);

            let bounced_tile_position = if !map.can_move_to(edge_tile_position.extend(position.z)) {
                // not on a walkable tile, bounce off the wall and keep going
                // rewind a bit to get back on the ice
                let new_ray_start = edge_tile_position - map.world_to_tile_space(ray * 2.0);

                // cast two rays
                let x_edge_tile_pos = cast_ray(new_ray_start, Vec2::new(ray.x, 0.0), is_not_ice);
                let y_edge_tile_pos = cast_ray(new_ray_start, Vec2::new(0.0, ray.y), is_not_ice);

                // pick the ray that moved us the furthest
                if new_ray_start.distance_squared(x_edge_tile_pos)
                    > new_ray_start.distance_squared(y_edge_tile_pos)
                {
                    Some(x_edge_tile_pos)
                } else {
                    Some(y_edge_tile_pos)
                }
            } else {
                None
            };

            // begin actor proeprty animation
            let mut property_animator = ActorPropertyAnimator::new();

            // begin with sfx and pause the player's animation for the slipping animation
            let mut first_steps = vec![(ActorProperty::AnimationSpeed(0.0), Ease::Floor)];

            if !sound_effect.is_empty() {
                first_steps.push((
                    ActorProperty::SoundEffect(sound_effect.to_string()),
                    Ease::Floor,
                ));
            }

            property_animator.add_key_frame(ActorKeyFrame {
                property_steps: first_steps,
                duration: 0.0,
            });

            // slide to first edge point
            property_animator.add_key_frame(ActorKeyFrame {
                property_steps: vec![
                    (ActorProperty::X(edge_position.x), Ease::Linear),
                    (ActorProperty::Y(edge_position.y), Ease::Linear),
                ],
                duration: start_tile_position.distance(edge_tile_position) / speed,
            });

            // slide to bounced position if it exists
            if let Some(final_tile_position) = bounced_tile_position {
                let final_position = map.tile_to_world(final_tile_position) + ray; // overshoot to get off the ice

                property_animator.add_key_frame(ActorKeyFrame {
                    property_steps: vec![
                        (ActorProperty::X(final_position.x), Ease::Linear),
                        (ActorProperty::Y(final_position.y), Ease::Linear),
                    ],
                    duration: edge_tile_position.distance(final_tile_position) / speed,
                });
            }

            let _ = entities.insert_one(player_data.entity, property_animator);
            ActorPropertyAnimator::start(game_io, assets, entities, player_data.entity);
        }
        TileClass::Treadmill => {
            let direction = map.screen_direction_to_world(tile.get_direction(tile_meta));
            let mut speed = tile_meta.custom_properties.get_f32("speed");

            if speed == 0.0 {
                speed = DEFAULT_TREADMILL_SPEED;
            }

            let movement = Vec2::from(direction.unit_vector()) * speed * 0.016666;
            *position = map.tile_3d_to_world(tile_position + movement.extend(0.0));
        }
        TileClass::Arrow | TileClass::Invisible | TileClass::Stairs | TileClass::Undefined => {}
    }
}

// based on: http://www.cse.yorku.ca/~amana/research/grid.pdf
// lua explanation: https://theshoemaker.de/2016/02/ray-casting-in-2d-grids/
fn cast_ray<F>(start: Vec2, ray: Vec2, test: F) -> Vec2
where
    // tile, position, ray
    F: Fn(IVec2, Vec2, Vec2) -> bool,
{
    let mut tile_position = start.as_ivec2();
    let step = IVec2::new(
        if ray.x < 0.0 { -1 } else { 1 },
        if ray.y < 0.0 { -1 } else { 1 },
    );

    // tMax* = the t distance to the next edge intersection
    let t_max_x_shift = if ray.x > 0.0 { 1.0 } else { 0.0 };
    let t_max_y_shift = if ray.y > 0.0 { 1.0 } else { 0.0 };

    let mut t_max_x = ((tile_position.x as f32 - start.x + t_max_x_shift) / ray.x).abs();
    let mut t_max_y = ((tile_position.y as f32 - start.y + t_max_y_shift) / ray.y).abs();

    // 0.0f / 0.0f = NaN and can break stuff
    t_max_x = if t_max_x.is_nan() {
        f32::INFINITY
    } else {
        t_max_x
    };

    t_max_y = if t_max_y.is_nan() {
        f32::INFINITY
    } else {
        t_max_y
    };

    // t = ray multiplier
    let mut t = 0.0;

    let mut current_position = start;

    if ray == Vec2::ZERO {
        return current_position;
    }

    loop {
        let taking_x_step = t_max_x < t_max_y;

        // increase t, reset tMax*, decrease the other tMax* as it is now closer, take a step
        if taking_x_step {
            t += t_max_x;
            t_max_y -= t_max_x;
            t_max_x = step.x as f32 / ray.x;
            tile_position.x += step.x;
        } else {
            t += t_max_y;
            t_max_x -= t_max_y;
            t_max_y = step.y as f32 / ray.y;
            tile_position.y += step.y;
        }

        current_position.x = start.x + ray.x * t;
        current_position.y = start.y + ray.y * t;

        if test(tile_position, current_position, ray) {
            break;
        }
    }

    current_position
}
