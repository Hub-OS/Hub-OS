use packets::structures::SpriteDefinition;

pub struct Sprite {
    pub definition: SpriteDefinition,
    pub public_sprites_index: Option<generational_arena::Index>,
    pub client_id_restriction: Option<String>,
}
