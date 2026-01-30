use crate::bindable::SpriteColorMode;
use crate::render::{Animator, AnimatorLoopMode, FrameTime, SpriteNode, Tree, TreeIndex};
use crate::resources::{Globals, LocalAssetManager};
use framework::prelude::{Color, GameIO, Vec2};

#[derive(Clone)]
pub struct AttackCharge {
    visible: bool,
    charging: bool,
    charging_time: FrameTime,
    max_charge_time: FrameTime,
    color: Color,
    sprite_index: TreeIndex,
    animator: Animator,
}

impl AttackCharge {
    pub const CHARGE_DELAY: FrameTime = 10;

    pub fn new(assets: &LocalAssetManager, sprite_index: TreeIndex, animation_path: &str) -> Self {
        Self {
            visible: true,
            charging: false,
            charging_time: 0,
            max_charge_time: 0,
            color: Color::BLACK,
            sprite_index,
            animator: Animator::load_new(assets, animation_path),
        }
    }

    pub fn create_sprite(
        game_io: &GameIO,
        sprite_tree: &mut Tree<SpriteNode>,
        texture_path: &str,
    ) -> TreeIndex {
        let sprite_index =
            sprite_tree.insert_root_child(SpriteNode::new(game_io, SpriteColorMode::Add));
        let charge_sprite = &mut sprite_tree[sprite_index];
        charge_sprite.set_texture(game_io, texture_path.to_string());
        charge_sprite.set_visible(false);
        charge_sprite.set_layer(-2);
        charge_sprite.set_offset(Vec2::new(0.0, -20.0));

        sprite_index
    }

    pub fn with_color(mut self, color: Color) -> Self {
        self.color = color;
        self
    }

    pub fn set_color(&mut self, color: Color) {
        self.color = color;
    }

    pub fn color(&self) -> Color {
        self.color
    }

    pub fn visible(&self) -> bool {
        self.visible
    }

    /// Overrides normal visibility, used to hide this sprite from opponents
    pub fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }

    pub fn has_charge(&self) -> bool {
        self.charging_time > 0
    }

    pub fn charging(&self) -> bool {
        self.charging
    }

    pub fn set_charging(&mut self, active: bool) {
        self.charging = active;
    }

    pub fn charging_time(&self) -> FrameTime {
        self.charging_time
    }

    pub fn fully_charged(&self) -> bool {
        self.charging_time >= self.max_charge_time + Self::CHARGE_DELAY
    }

    pub fn max_charge_time(&self) -> FrameTime {
        self.max_charge_time
    }

    pub fn set_max_charge_time(&mut self, time: FrameTime) {
        self.max_charge_time = time
    }

    pub fn cancel(&mut self) {
        self.charging_time = 0;
        self.charging = false;
    }

    pub fn sprite_offset(&self, sprite_tree: &Tree<SpriteNode>) -> Vec2 {
        sprite_tree[self.sprite_index].offset()
    }

    pub fn update_sprite_offset(&self, sprite_tree: &mut Tree<SpriteNode>, offset: Vec2) {
        sprite_tree[self.sprite_index].set_offset(offset);
    }

    pub fn update_sprite(&mut self, sprite_tree: &mut Tree<SpriteNode>) {
        let sprite_node = sprite_tree.get_mut(self.sprite_index).unwrap();

        if self.charging_time < Self::CHARGE_DELAY {
            sprite_node.set_visible(false);
            return;
        }

        self.animator.update();

        if self.charging_time >= self.max_charge_time + Self::CHARGE_DELAY {
            // charged
            if self.animator.current_state() != Some("CHARGED") {
                self.animator.set_state("CHARGED");
                self.animator.set_loop_mode(AnimatorLoopMode::Loop);
            }

            sprite_node.set_color(self.color);
        } else {
            // charging
            if self.animator.current_state() != Some("CHARGING") {
                self.animator.set_state("CHARGING");
                self.animator.set_loop_mode(AnimatorLoopMode::Loop);
            }

            sprite_node.set_color(Color::BLACK);
        }

        sprite_node.apply_animation(&self.animator);
        sprite_node.set_visible(self.visible);
    }

    pub fn update(&mut self, game_io: &GameIO, play_sfx: bool) -> Option<bool> {
        let previously_charging = self.has_charge();

        if self.charging {
            // charging
            if play_sfx && self.charging_time == Self::CHARGE_DELAY {
                let globals = Globals::from_resources(game_io);
                globals.audio.play_sound(&globals.sfx.attack_charging);
            }

            if play_sfx && self.charging_time == self.max_charge_time + Self::CHARGE_DELAY {
                let globals = Globals::from_resources(game_io);
                globals.audio.play_sound(&globals.sfx.attack_charged);
            }

            self.charging_time += 1;
        } else if previously_charging {
            // shooting
            let fully_charged = self.fully_charged();

            return Some(fully_charged);
        }

        None
    }
}
