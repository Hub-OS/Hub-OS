use crate::render::{Animator, SpriteColorQueue};
use crate::{AssetManager, Globals};
use framework::common::GameIO;
use framework::prelude::{Sprite, Vec2};
use packets::structures::Emotion;
use std::sync::Arc;

#[derive(Clone)]
pub struct EmotionUi {
    emotion: Emotion,
    texture_path: Arc<str>,
    animation_path: Arc<str>,
    sprite: Sprite,
    animator: Animator,
}

impl EmotionUi {
    pub fn new(
        game_io: &GameIO,
        mut emotion: Emotion,
        texture_path: &str,
        animation_path: &str,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // load sprite
        let sprite = assets.new_sprite(game_io, texture_path);

        // load animation
        let mut animator = Animator::load_new(assets, animation_path);

        if !animator.has_state(emotion.as_str()) {
            // use the default emotion if the provided emotion isn't supported
            emotion = Emotion::default();
        }

        animator.set_state(emotion.as_str());

        Self {
            emotion,
            texture_path: texture_path.into(),
            animation_path: animation_path.into(),
            sprite,
            animator,
        }
    }

    pub fn texture_path(&self) -> &str {
        &self.texture_path
    }

    pub fn animation_path(&self) -> &str {
        &self.animation_path
    }

    pub fn set_texture(&mut self, game_io: &GameIO, path: String) {
        let globals = game_io.resource::<Globals>().unwrap();

        let texture = globals.assets.texture(game_io, &path);
        self.sprite.set_texture(texture);

        self.texture_path = path.into();
    }

    pub fn load_animation(&mut self, game_io: &GameIO, path: String) {
        let globals = game_io.resource::<Globals>().unwrap();
        self.animator.load(&globals.assets, &path);
        self.animation_path = path.into();

        if !self.animator.has_state(self.emotion.as_str()) {
            self.emotion = Emotion::default();
            self.animator.set_state(self.emotion.as_str());
        }
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.sprite.set_position(position);
    }

    pub fn emotions(&self) -> impl Iterator<Item = &str> {
        self.animator.iter_states().map(|(s, _)| s)
    }

    pub fn emotion(&self) -> &Emotion {
        &self.emotion
    }

    pub fn has_emotion(&self, emotion: &Emotion) -> bool {
        self.animator.has_state(emotion.as_str())
    }

    pub fn set_emotion(&mut self, emotion: Emotion) {
        if self.animator.has_state(emotion.as_str()) {
            // only apply the emotion if it's supported
            self.animator.set_state(emotion.as_str());
            self.emotion = emotion;
        }
    }

    pub fn update(&mut self) {
        self.animator.update();
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.animator.apply(&mut self.sprite);
        sprite_queue.draw_sprite(&self.sprite);
    }
}
