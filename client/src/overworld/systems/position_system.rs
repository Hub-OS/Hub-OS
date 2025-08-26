use crate::overworld::components::*;
use crate::overworld::OverworldArea;
use crate::render::ui::Text;

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

    for (_, (world_position, text)) in entities.query_mut::<(&Vec3, &mut Text)>() {
        let bounds = &mut text.style.bounds;
        let position = map.world_3d_to_screen(*world_position).floor();
        bounds.set_position(position);
    }

    // move attachment sprites to anchor points
    for (_, (sprite, text, attachment, offset)) in entities
        .query::<(
            Option<&mut Sprite>,
            Option<&mut Text>,
            &ActorAttachment,
            &Vec2,
        )>()
        .into_iter()
    {
        let mut total_offset = *offset;

        if let Some(point_name) = &attachment.point {
            let Ok(mut query) = entities.query_one::<&Animator>(attachment.actor_entity) else {
                log::error!("A sprite attachment exists without parent actor");
                continue;
            };

            if let Some(animator) = query.get() {
                total_offset += resolve_actor_point(animator, point_name);
            }
        }

        if let Some(sprite) = sprite {
            sprite.set_position(sprite.position() + total_offset);
        }

        if let Some(text) = text {
            let bounds = &mut text.style.bounds;
            bounds.set_position(bounds.position() + total_offset);
        }
    }

    // fix text alignment
    for (_, (text, alignment, _)) in
        entities.query_mut::<(&mut Text, &AttachmentAlignment, &ActorAttachment)>()
    {
        alignment.align_text(text);
    }
}

fn resolve_actor_point(animator: &Animator, name: &str) -> Vec2 {
    if let Some(point) = animator.point(name) {
        return point - animator.origin();
    }

    match name.to_uppercase().as_str() {
        "EMOTE" => {
            // use the first frame of the IDLE_D or DL state to decide placement
            // targeting top center
            let frame_list = animator
                .frame_list("IDLE_D")
                .or_else(|| animator.frame_list("IDLE_DL"));

            let point_y = frame_list
                .and_then(|list| list.frames().first())
                .map(|frame| -frame.origin.y)
                .unwrap_or(-animator.origin().y);

            Vec2::new(0.0, point_y)
        }
        _ => -animator.origin(),
    }
}
