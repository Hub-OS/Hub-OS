use crate::overworld::components::*;
use crate::scenes::OverworldSceneBase;

const WARP_IN_REVEAL_FRAME: usize = 5;
const WARP_OUT_HIDE_FRAME: usize = 2;

pub fn system_warp_effect(scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;
    let mut pending_action = Vec::new();
    let mut pending_removal = Vec::new();

    for (entity, (effect, animator)) in entities.query::<(&mut WarpEffect, &mut Animator)>().iter()
    {
        let current_frame = animator.current_frame_index();
        let frame_changed = effect.last_frame != current_frame;
        effect.last_frame = current_frame;

        if frame_changed {
            let is_hidden = entities
                .query_one::<&HiddenSprite>(effect.actor_entity)
                .is_ok();

            match effect.warp_type {
                WarpType::In { .. } => {
                    if current_frame == WARP_IN_REVEAL_FRAME && is_hidden {
                        pending_action.push((effect.warp_type, effect.actor_entity));
                    }
                }
                WarpType::Out | WarpType::Full { .. } => {
                    if current_frame == WARP_OUT_HIDE_FRAME && !is_hidden {
                        pending_action.push((effect.warp_type, effect.actor_entity));
                    }
                }
            }
        }

        if !animator.is_complete() {
            continue;
        }

        if let WarpType::Full {
            position,
            direction,
        } = effect.warp_type
        {
            effect.warp_type = WarpType::In {
                position,
                direction,
            };
            animator.set_state("IN");

            // todo: play sfx?
        } else {
            pending_removal.push((entity, effect.actor_entity));
        }
    }

    for (entity, actor_entity) in pending_removal {
        let _ = entities.despawn(entity);

        if let Ok(mut warp_controller) = entities.get::<&mut WarpController>(actor_entity) {
            warp_controller.warp_entity = None;
        }
    }

    for (warp_type, actor_entity) in pending_action {
        match warp_type {
            WarpType::In {
                position,
                direction,
            } => {
                let _ = entities.remove_one::<HiddenSprite>(actor_entity);

                if let Ok(mut query) =
                    entities.query_one::<(&mut Vec3, &mut Direction)>(actor_entity)
                {
                    let (set_position, set_direction) = query.get().unwrap();

                    *set_position = position;
                    *set_direction = direction;
                }
            }
            WarpType::Out | WarpType::Full { .. } => {
                let _ = entities.insert_one(actor_entity, HiddenSprite);
            }
        }
    }
}
