use crate::overworld::{components::*, Map, ObjectData};
use crate::render::FrameTime;
use crate::scenes::OverworldSceneBase;
use framework::prelude::{GameIO, Vec3Swizzles};

const WARP_IN_REVEAL_FRAME: usize = 4;
const WARP_OUT_HIDE_FRAME: usize = 0;
const WALK_OUT_TIME: FrameTime = 8;

pub fn system_warp_effect(game_io: &mut GameIO, scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;
    let map = &mut scene.map;
    let mut pending_action = Vec::new();
    let mut pending_removal = Vec::new();

    for (entity, (effect, animator)) in entities.query::<(&mut WarpEffect, &mut Animator)>().iter()
    {
        let current_frame = animator.current_frame_index();
        let frame_changed = effect.last_frame != Some(current_frame);
        effect.last_frame = Some(current_frame);

        if frame_changed {
            match effect.warp_type {
                WarpType::In { .. } => {
                    if current_frame == WARP_IN_REVEAL_FRAME {
                        pending_action.push((
                            effect.warp_type,
                            effect.actor_entity,
                            effect.callback.take(),
                        ));
                    }
                }
                WarpType::Out | WarpType::Full { .. } => {
                    if current_frame == WARP_OUT_HIDE_FRAME {
                        // only passing callback for WarpType::Out as
                        // WarpType::Full truely completes when it becomes a WarpType::In
                        let callback = if matches!(effect.warp_type, WarpType::Out) {
                            effect.callback.take()
                        } else {
                            None
                        };

                        pending_action.push((effect.warp_type, effect.actor_entity, callback));
                    }
                }
            }
        }

        if !animator.is_complete() {
            continue;
        }

        match effect.warp_type {
            WarpType::In { direction, .. } => {
                let actor_entity = effect.actor_entity;

                if handle_walk_out(map, entities, actor_entity, direction) {
                    // skip deleting until the walk animation has completed
                    continue;
                }

                if let Ok(mut warp_controller) = entities.get::<&mut WarpController>(actor_entity) {
                    warp_controller.warped_out = false;
                }
            }
            WarpType::Out => {}
            WarpType::Full {
                position,
                direction,
            } => {
                effect.warp_type = WarpType::In {
                    position,
                    direction,
                };
                animator.set_state("IN");

                // todo: play sfx?

                continue;
            }
        }

        pending_removal.push((entity, effect.actor_entity));
    }

    for (entity, actor_entity) in pending_removal {
        let _ = entities.despawn(entity);

        if let Ok(mut warp_controller) = entities.get::<&mut WarpController>(actor_entity) {
            warp_controller.warp_entity = None;
        }
    }

    for (warp_type, actor_entity, callback) in pending_action {
        let entities = &mut scene.entities;

        match warp_type {
            WarpType::In {
                position,
                direction,
            } => {
                Excluded::decrement(entities, actor_entity);

                let Ok(components) = entities.query_one_mut::<(&mut Vec3, &mut Direction)>(actor_entity) else {
                    continue;
                };

                let (set_position, set_direction) = components;

                *set_position = position;

                if !direction.is_none() {
                    *set_direction = direction;
                }

                if let Some(callback) = callback {
                    callback(game_io, scene);
                }
            }
            WarpType::Out | WarpType::Full { .. } => {
                Excluded::increment(entities, actor_entity);

                if entities.contains(actor_entity) {
                    if let Some(callback) = callback {
                        callback(game_io, scene);
                    }
                }
            }
        }
    }
}

fn handle_walk_out(
    map: &mut Map,
    entities: &hecs::World,
    actor_entity: hecs::Entity,
    mut direction: Direction,
) -> bool {
    let Ok(mut components) = entities.query_one::<(&Vec3, &Direction, &mut WarpController, &mut MovementAnimator)>(actor_entity) else {
        return false;
    };

    let Some((position, player_direction, warp_controller, movement_animator)) = components.get() else {
        return false;
    };

    if direction.is_none() {
        direction = *player_direction;
    }

    if let Some(object_entity) = map.tile_object_at(position.xy(), position.z as i32, false) {
        let data = map
            .object_entities_mut()
            .query_one_mut::<&ObjectData>(object_entity)
            .unwrap();

        if data.object_type.is_warp() {
            // reset the timer as long as we're inside a warp
            warp_controller.walk_timer = Some(0);
        }
    }

    let Some(timer) = warp_controller.walk_timer.as_mut() else {
        return false;
    };

    if movement_animator.movement_enabled() {
        movement_animator.queue_direction(direction);
        movement_animator.set_state(MovementState::Walking);
    }

    *timer += 1;

    if *timer > WALK_OUT_TIME {
        warp_controller.walk_timer = None;
        return false;
    }

    true
}
