pub struct HudAttachment;
pub struct WidgetAttachment;
pub struct AttachmentLayer(pub i32);

pub struct ActorAttachment {
    pub actor_entity: hecs::Entity,
    pub point: Option<String>,
}
