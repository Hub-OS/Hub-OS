use packets::{
    create_generational_index,
    structures::{ActorId, SpriteDefinition},
};

create_generational_index!(PublicSpriteId);

pub struct Sprite {
    pub definition: SpriteDefinition,
    pub public_sprite_id: Option<PublicSpriteId>,
    pub client_id_restriction: Option<ActorId>,
}
