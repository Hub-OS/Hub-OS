use super::{FontStyle, Text};
use crate::render::*;
use crate::resources::*;
use framework::prelude::{GameIO, Sprite, Vec2};

#[derive(Clone)]
pub struct PlayerHealthUI {
    current_health: i32,
    target_health: i32,
    style_change_cooldown: FrameTime,
    text: Text,
    text_offset: Vec2,
    frame_sprite: Sprite,
}

impl PlayerHealthUI {
    pub fn new(game_io: &GameIO<Globals>) -> Self {
        let globals = game_io.globals();
        let assets = &globals.assets;

        // use animation to load placement information
        let mut animator = Animator::load_new(assets, ResourcePaths::HEALTH_FRAME_ANIMATION);
        animator.set_state("DEFAULT");
        let text_offset = animator.point("TEXT_START").unwrap_or_default();

        let mut health_ui = Self {
            current_health: 0,
            target_health: 0,
            style_change_cooldown: 0,
            text: Text::new_monospace(game_io, FontStyle::Gradient),
            text_offset,
            frame_sprite: assets.new_sprite(game_io, ResourcePaths::HEALTH_FRAME),
        };

        health_ui.set_position(Vec2::new(BATTLE_UI_MARGIN, 0.0));

        health_ui
    }

    pub fn set_health(&mut self, health: i32) {
        self.target_health = health;
    }

    pub fn snap_health(&mut self, health: i32) {
        self.current_health = health;
        self.target_health = health;
        self.text.text = format!("{:>4}", self.current_health);
    }

    pub fn position(&mut self) -> Vec2 {
        self.frame_sprite.position()
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.frame_sprite.set_position(position);
        (self.text.style.bounds).set_position(position + self.text_offset);
    }

    pub fn update(&mut self) {
        if self.style_change_cooldown > 0 {
            self.style_change_cooldown -= 1;
        }

        if self.style_change_cooldown == 0 {
            self.text.style.font_style = FontStyle::Gradient;
        }

        if self.current_health == self.target_health {
            return;
        }

        let diff = self.target_health - self.current_health;

        let increment = match diff.abs() {
            diff if diff >= 100 => 10,
            diff if diff >= 40 => 5,
            diff if diff >= 3 => 3,
            _ => 1,
        };

        self.current_health += diff.signum() * increment;

        if diff < 0 {
            self.style_change_cooldown = 15; // quarter of a second
            self.text.style.font_style = FontStyle::GradientGold;
        } else {
            self.text.style.font_style = FontStyle::GradientGreen;
        }

        self.text.text = format!("{:>4}", self.current_health);
    }

    pub fn draw(&self, game_io: &GameIO<Globals>, sprite_queue: &mut SpriteColorQueue) {
        sprite_queue.draw_sprite(&self.frame_sprite);
        self.text.draw(game_io, sprite_queue);
    }
}
