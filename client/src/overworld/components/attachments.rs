use framework::prelude::*;
use packets::structures::{ActorId, SpriteAttachment, SpriteParent, TextAxisAlignment};

use crate::render::ui::Text;

pub struct HudAttachment;
pub struct WidgetAttachment;
pub struct AttachmentLayer(pub i32);

pub struct AttachmentAlignment(pub TextAxisAlignment, pub TextAxisAlignment);

impl AttachmentAlignment {
    pub fn align_text(&self, text: &mut Text) {
        let size = text.measure().size;
        let bounds = &mut text.style.bounds;
        let offset = Vec2::new(
            match self.0 {
                TextAxisAlignment::Start => 0.0,
                TextAxisAlignment::Middle => -size.x * 0.5,
                TextAxisAlignment::End => -size.x,
            },
            match self.1 {
                TextAxisAlignment::Start => 0.0,
                TextAxisAlignment::Middle => -size.y * 0.5,
                TextAxisAlignment::End => -size.y,
            },
        );

        *bounds += offset;
    }
}

pub struct ActorAttachment {
    pub actor_entity: hecs::Entity,
    pub point: Option<String>,
}

pub fn insert_attachment_bundle(
    entities: &mut hecs::World,
    actor_id_map: &bimap::BiHashMap<ActorId, hecs::Entity>,
    attachment: SpriteAttachment,
    entity: hecs::Entity,
) {
    let offset = Vec2::new(attachment.x, attachment.y);

    let layer = AttachmentLayer(if attachment.layer <= 0 {
        // shift 0 and lower by -1, forces layer 0 to always display in front of the parent
        attachment.layer - 1
    } else {
        attachment.layer
    });

    let _ = entities.insert(entity, (layer, offset));

    if let Ok(sprite) = entities.query_one_mut::<&mut Sprite>(entity) {
        sprite.set_position(offset);
    }

    if let Ok((text, alignment)) =
        entities.query_one_mut::<(&mut Text, &AttachmentAlignment)>(entity)
    {
        text.style.bounds.set_position(offset);
        alignment.align_text(text);
    }

    let _ = match attachment.parent {
        SpriteParent::Widget => entities.insert_one(entity, WidgetAttachment),
        SpriteParent::Hud => entities.insert_one(entity, HudAttachment),
        SpriteParent::Actor(actor_id) => {
            let attachment = ActorAttachment {
                actor_entity: actor_id_map
                    .get_by_left(&actor_id)
                    .cloned()
                    .unwrap_or(hecs::Entity::DANGLING),
                point: attachment.parent_point,
            };

            entities.insert(entity, (attachment, Vec3::ZERO))
        }
    };
}
