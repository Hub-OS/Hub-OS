use super::{FontName, Text};
use crate::render::*;
use crate::resources::*;
use framework::prelude::{GameIO, Rect, Sprite, Vec2};

#[derive(Clone)]
pub struct PlayerHealthUi {
    current_health: i32,
    target_health: i32,
    max_health: i32,
    style_change_cooldown: FrameTime,
    text: Text,
    text_offset: Vec2,
    frame_sprite: Sprite,
}

impl PlayerHealthUi {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        // use animation to load placement information
        let mut animator = Animator::load_new(assets, ResourcePaths::HEALTH_FRAME_ANIMATION);
        animator.set_state("DEFAULT");
        let text_offset = animator.point_or_zero("TEXT_START");

        let mut health_ui = Self {
            max_health: 0,
            current_health: 0,
            target_health: 0,
            style_change_cooldown: 0,
            text: Text::new_monospace(game_io, FontName::PlayerHp),
            text_offset,
            frame_sprite: assets.new_sprite(game_io, ResourcePaths::HEALTH_FRAME),
        };

        health_ui.set_position(Vec2::new(BATTLE_UI_MARGIN, 0.0));

        health_ui
    }

    pub fn with_health(mut self, health: i32) -> Self {
        self.snap_health(health);
        self
    }

    pub fn with_max_health(mut self, health: i32) -> Self {
        self.max_health = health;
        self
    }

    pub fn set_health(&mut self, health: i32) {
        self.target_health = health;
    }

    pub fn set_max_health(&mut self, health: i32) {
        self.max_health = health;
    }

    pub fn is_low_hp(&self) -> bool {
        self.current_health <= self.max_health / 5
    }

    pub fn snap_health(&mut self, health: i32) {
        self.current_health = health;
        self.target_health = health;
        self.text.text = format!("{:>4}", self.current_health);
    }

    pub fn bounds(&self) -> Rect {
        self.frame_sprite.bounds()
    }

    pub fn position(&self) -> Vec2 {
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

        if self.is_low_hp() {
            self.text.style.font = FontName::PlayerHpOrange;
        } else if self.style_change_cooldown == 0 {
            self.text.style.font = FontName::PlayerHp;
        }

        if self.current_health == self.target_health {
            return;
        }

        let diff = self.target_health - self.current_health;

        // https://www.desmos.com/calculator/qwxqhthmuw
        // x = diff, y = change
        let y = match diff.abs() {
            x @ 4.. => x / 8 + 4,
            2..4 => 2,
            _ => 1,
        };

        self.current_health += diff.signum() * y;

        if diff < 0 {
            self.style_change_cooldown = 15; // quarter of a second
            self.text.style.font = FontName::PlayerHpOrange;
        } else {
            self.text.style.font = FontName::PlayerHpGreen;
        }

        self.text.text = format!("{:>4}", self.current_health);
    }

    pub fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        sprite_queue.draw_sprite(&self.frame_sprite);
        self.text.draw(game_io, sprite_queue);
    }
}
