use super::{ActorAttachment, Animator, AttachmentLayer, MovementAnimator};
use crate::overworld::OverworldArea;
use crate::render::{AnimatorLoopMode, FrameTime};
use framework::prelude::*;

const MAX_LIFETIME: FrameTime = 60 * 5;

pub struct Emote {
    pub lifetime: FrameTime,
    pub offset: Vec2,
}

impl Emote {
    pub fn animate_actor(
        entities: &mut hecs::World,
        entity: hecs::Entity,
        state: &str,
        loop_animation: bool,
    ) {
        let (animator, movement_animator) = entities
            .query_one_mut::<(&mut Animator, &mut MovementAnimator)>(entity)
            .unwrap();

        if !animator.has_state(state) {
            return;
        }

        animator.set_state(state);

        if loop_animation {
            animator.set_loop_mode(AnimatorLoopMode::Loop);
        }

        movement_animator.set_animation_enabled(false);
    }

    pub fn spawn_or_recycle(area: &mut OverworldArea, parent_entity: hecs::Entity, emote_id: &str) {
        let entities = &mut area.entities;

        if !area.emote_animator.has_state(emote_id) {
            // despawn any existing emote
            for (id, (_, attachment, _)) in
                entities.query_mut::<(&mut Emote, &ActorAttachment, &mut Animator)>()
            {
                if attachment.actor_entity != parent_entity {
                    continue;
                }

                let _ = entities.despawn(id);
                break;
            }

            return;
        }

        let initial_offset = Self::resolve_offset(entities, parent_entity).unwrap_or_else(|| {
            let Ok(parent_animator) = entities.query_one_mut::<&Animator>(parent_entity) else {
                return Vec2::ZERO;
            };

            Vec2::new(0.0, -parent_animator.origin().y)
        });

        for (_, (emote, attachment, animator, offset)) in
            entities.query_mut::<(&mut Emote, &ActorAttachment, &mut Animator, &mut Vec2)>()
        {
            if attachment.actor_entity != parent_entity {
                continue;
            }

            // update existing emote
            emote.lifetime = 0;
            animator.set_state(emote_id);
            animator.set_loop_mode(AnimatorLoopMode::Loop);

            // update offset
            *offset -= emote.offset;
            emote.offset = animator.point_or_zero("OFFSET");
            *offset += emote.offset;

            return;
        }

        Self::spawn(area, parent_entity, emote_id, initial_offset);
    }

    fn spawn(
        area: &mut OverworldArea,
        parent_entity: hecs::Entity,
        emote_id: &str,
        initial_offset: Vec2,
    ) {
        let mut animator = area.emote_animator.clone();
        animator.set_state(emote_id);
        animator.set_loop_mode(AnimatorLoopMode::Loop);

        let sprite = area.emote_sprite.clone();
        let frame_offset = animator.point_or_zero("OFFSET");
        let offset = initial_offset + frame_offset;

        let attachment_layer = AttachmentLayer(-1);
        let attachment = ActorAttachment {
            actor_entity: parent_entity,
            point: None,
        };

        area.entities.spawn((
            Emote {
                lifetime: 0,
                offset: frame_offset,
            },
            animator,
            sprite,
            offset,
            attachment_layer,
            attachment,
            Vec3::ZERO,
        ));
    }

    pub fn system(area: &mut OverworldArea) {
        // handle timers
        let mut pending_deletion = Vec::new();

        let entities = &mut area.entities;

        for (entity, emote) in entities.query_mut::<&mut Emote>() {
            emote.lifetime += 1;

            if emote.lifetime > MAX_LIFETIME {
                pending_deletion.push(entity);
            }
        }

        for entity in pending_deletion {
            let _ = entities.despawn(entity);
        }

        // move to "EMOTE" point
        let mut emote_query = entities.query::<(&Emote, &ActorAttachment, &mut Vec2)>();
        for (_, (emote, attachment, offset)) in emote_query.into_iter() {
            if let Some(new_offset) = Self::resolve_offset(entities, attachment.actor_entity) {
                *offset = new_offset + emote.offset;
            }
        }
    }

    fn resolve_offset(entities: &hecs::World, parent_entity: hecs::Entity) -> Option<Vec2> {
        let Ok(mut parent_query) = entities.query_one::<&Animator>(parent_entity) else {
            return None;
        };

        let parent_animator = parent_query.get()?;

        parent_animator
            .point("EMOTE")
            .map(|point| point - parent_animator.origin())
    }
}
