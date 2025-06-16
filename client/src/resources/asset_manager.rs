use super::SoundBuffer;
use crate::render::ui::GlyphAtlas;
use framework::prelude::*;
use std::sync::Arc;

pub trait AssetManager {
    fn local_path(&self, path: &str) -> String;
    fn binary(&self, path: &str) -> Vec<u8>;
    fn binary_silent(&self, path: &str) -> Vec<u8>;
    fn text(&self, path: &str) -> String;
    fn text_silent(&self, path: &str) -> String;
    fn texture(&self, game_io: &GameIO, path: &str) -> Arc<Texture>;
    fn audio(&self, game_io: &GameIO, path: &str) -> SoundBuffer;
    fn glyph_atlas(
        &self,
        game_io: &GameIO,
        texture_path: &str,
        animation_path: &str,
    ) -> Arc<GlyphAtlas>;

    fn new_sprite(&self, game_io: &GameIO, texture_path: &str) -> Sprite {
        let texture = self.texture(game_io, texture_path);

        Sprite::new(game_io, texture)
    }
}
