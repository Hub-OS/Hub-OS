use packets::structures::SpriteDefinition;

pub struct Sprite {
    pub definition: SpriteDefinition,
    pub public_sprites_index: Option<slotmap::DefaultKey>,
    pub client_id_restriction: Option<String>,
}
