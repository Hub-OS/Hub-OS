use crate::bindable::{Element, SpriteColorMode};
use crate::render::SpriteNode;
use crate::resources::*;
use framework::prelude::*;

pub struct ElementSprite;

impl ElementSprite {
    #[allow(clippy::new_ret_no_self)]
    pub fn new(game_io: &GameIO, element: Element) -> Sprite {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::ELEMENTS);

        sprite.set_frame(Rect::new((element as u8 as f32) * 14.0, 0.0, 14.0, 14.0));

        sprite
    }

    pub fn new_node(game_io: &GameIO, element: Element) -> SpriteNode {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut sprite_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);

        let texture = assets.texture(game_io, ResourcePaths::ELEMENTS);
        sprite_node.set_texture_direct(texture);

        sprite_node.set_frame(Rect::new((element as u8 as f32) * 14.0, 0.0, 14.0, 14.0));

        sprite_node
    }
}
