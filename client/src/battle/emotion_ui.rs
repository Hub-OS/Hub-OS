use crate::render::{Animator, SpriteColorQueue};
use framework::prelude::{Sprite, Vec2};
use packets::structures::Emotion;

#[derive(Clone)]
pub struct EmotionUi {
    emotion: Emotion,
    sprite: Sprite,
    animator: Animator,
}

impl EmotionUi {
    pub fn new(mut emotion: Emotion, sprite: Sprite, mut animator: Animator) -> Self {
        if !animator.has_state(emotion.as_str()) {
            // use the default emotion if the provided emotion isn't supported
            emotion = Emotion::default();
        }

        animator.set_state(emotion.as_str());

        Self {
            emotion,
            sprite,
            animator,
        }
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.sprite.set_position(position);
    }

    pub fn emotions(&self) -> impl Iterator<Item = &str> {
        self.animator.iter_states().map(|(s, _)| s.as_str())
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
