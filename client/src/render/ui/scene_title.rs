use super::*;
use crate::render::*;
use crate::resources::{Globals, TranslationArgs, TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;

pub struct SceneTitle {
    title: String,
}
impl SceneTitle {
    pub fn new(game_io: &GameIO, title_translation_key: &str) -> Self {
        let globals = Globals::from_resources(game_io);
        let title = globals.translate(title_translation_key);
        Self { title }
    }

    pub fn new_with_args(
        game_io: &GameIO,
        title_translation_key: &str,
        args: TranslationArgs,
    ) -> Self {
        let globals = Globals::from_resources(game_io);
        let title = globals.translate_with_args(title_translation_key, args);
        Self { title }
    }

    pub fn set_title(
        &mut self,
        game_io: &GameIO,
        title_translation_key: &str,
        args: TranslationArgs,
    ) {
        let globals = Globals::from_resources(game_io);
        self.title = globals.translate_with_args(title_translation_key, args);
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let mut style = TextStyle::new_monospace(game_io, FontName::Code);
        style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        style.bounds.set_position(Vec2::new(16.0, 6.0));
        style.draw(game_io, sprite_queue, &self.title);
    }
}
