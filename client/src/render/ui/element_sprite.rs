use crate::bindable::Element;
use crate::resources::*;
use framework::prelude::*;

pub struct ElementSprite;

impl ElementSprite {
    #[allow(clippy::new_ret_no_self)]
    pub fn new(game_io: &GameIO<Globals>, element: Element) -> Sprite {
        let globals = game_io.globals();
        let assets = &globals.assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::ELEMENTS);

        sprite.set_frame(Rect::new((element as u8 as f32) * 14.0, 0.0, 14.0, 14.0));

        sprite
    }
}
