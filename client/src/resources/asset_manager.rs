use framework::prelude::*;
use std::sync::Arc;

#[derive(Clone)]
pub struct SoundBuffer(pub Arc<Vec<u8>>);

impl AsRef<[u8]> for SoundBuffer {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

pub trait AssetManager {
    fn local_path(&self, path: &str) -> String;
    fn binary(&self, path: &str) -> Vec<u8>;
    fn text(&self, path: &str) -> String;
    fn texture(&self, game_io: &GameIO, path: &str) -> Arc<Texture>;
    fn audio(&self, path: &str) -> SoundBuffer;

    fn new_sprite(&self, game_io: &GameIO, texture_path: &str) -> Sprite {
        let texture = self.texture(game_io, texture_path);

        Sprite::new(game_io, texture)
    }
}
