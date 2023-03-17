use crate::overworld::components::*;
use crate::overworld::OverworldArea;

pub fn system_position(area: &mut OverworldArea) {
    let entities = &mut area.entities;
    let map = &area.map;

    // update position for attachments
    for (_, (position, attachment, &AttachmentLayer(layer))) in entities
        .query::<(&mut Vec3, &ActorAttachment, &AttachmentLayer)>()
        .into_iter()
    {
        let Ok(mut query) = entities.query_one::<&Vec3>(attachment.actor_entity) else {
            log::error!("A sprite attachment exists without parent actor");
            continue;
        };

        if let Some(&parent_position) = query.get() {
            *position = parent_position;

            // nudge for proper layering
            let layer_offset = -layer as f32 * 0.001;
            position.x += layer_offset;
            position.y += layer_offset;
        }
    }

    // update sprite positions based on world position
    for (_, (world_position, sprite)) in entities.query_mut::<(&Vec3, &mut Sprite)>() {
        let position = map.world_3d_to_screen(*world_position).floor();
        sprite.set_position(position);
    }

    // move attachment sprites to anchor points
    for (_, (sprite, attachment)) in entities
        .query::<(&mut Sprite, &ActorAttachment)>()
        .into_iter()
    {
        let Some(point_name) = &attachment.point else {
            continue;
        };

        let Ok(mut query) = entities.query_one::<&Animator>(attachment.actor_entity) else {
            log::error!("A sprite attachment exists without parent actor");
            continue;
        };

        if let Some(animator) = query.get() {
            let point_offset = animator.point(point_name).unwrap_or_default() - animator.origin();
            sprite.set_position(sprite.position() + point_offset);
        }
    }

    // apply attachment offsets
    for (_, (sprite, &offset, _)) in entities
        .query::<(&mut Sprite, &Vec2, &ActorAttachment)>()
        .into_iter()
    {
        sprite.set_position(sprite.position() + offset);
    }
}
