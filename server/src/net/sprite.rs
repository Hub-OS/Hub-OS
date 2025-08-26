use packets::create_generational_index;
use packets::structures::{ActorId, SpriteAttachment, SpriteDefinition};

create_generational_index!(PublicSpriteId);

pub struct Sprite {
    pub attachment: SpriteAttachment,
    pub definition: SpriteDefinition,
    pub public_sprite_id: Option<PublicSpriteId>,
    pub client_id_restriction: Option<ActorId>,
}
