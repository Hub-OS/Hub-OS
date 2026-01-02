use crate::render::*;
use crate::resources::{AssetManager, Globals, RESOLUTION_F, ResourcePaths};
use framework::prelude::{GameIO, Rect, Sprite, Vec2};

#[derive(Clone)]
pub struct TurnGauge {
    enabled: bool,
    time: FrameTime,
    max_time: FrameTime,
    animator: Animator,
    container_sprite: Sprite,
    bar_sprite: Sprite,
    completed_turn: bool,
}

impl TurnGauge {
    pub const DEFAULT_MAX_TIME: FrameTime = 512;

    pub fn new(game_io: &GameIO) -> Self {
        let assets = &Globals::from_resources(game_io).assets;

        let sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_TURN_GAUGE);
        let mut container_sprite = sprite.clone();
        container_sprite.set_position(Vec2::new(RESOLUTION_F.x * 0.5, 0.0));

        let mut animator = Animator::load_new(assets, ResourcePaths::BATTLE_TURN_GAUGE_ANIMATION);
        animator.set_state("CONTAINER");
        animator.apply(&mut container_sprite);

        Self {
            enabled: true,
            time: 0,
            max_time: Self::DEFAULT_MAX_TIME,
            animator,
            bar_sprite: sprite,
            container_sprite,
            completed_turn: false,
        }
    }

    pub fn enabled(&self) -> bool {
        self.enabled
    }

    pub fn set_enabled(&mut self, enabled: bool) {
        self.enabled = enabled;
    }

    pub fn increment_time(&mut self) {
        if self.enabled {
            self.time = self.max_time.min(self.time + 1);
        }

        self.animator.update();
    }

    pub fn set_completed_turn(&mut self, value: bool) {
        self.completed_turn = value && self.enabled;
    }

    pub fn completed_turn(&self) -> bool {
        self.completed_turn
    }

    pub fn is_complete(&self) -> bool {
        self.enabled && (self.time >= self.max_time || self.completed_turn)
    }

    pub fn progress(&self) -> f32 {
        if self.max_time == 0 {
            1.0
        } else {
            self.time as f32 / self.max_time as f32
        }
    }

    pub fn time(&self) -> FrameTime {
        self.time
    }

    pub fn set_time(&mut self, time: FrameTime) {
        self.time = time.min(self.max_time)
    }

    pub fn max_time(&self) -> FrameTime {
        self.max_time
    }

    pub fn set_max_time(&mut self, time: FrameTime) {
        self.max_time = time
    }

    pub fn bounds(&self) -> Rect {
        self.container_sprite.bounds()
    }

    fn resolve_state(&self) -> &'static str {
        if self.is_complete() {
            "READY"
        } else {
            "STRETCH"
        }
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        if !self.enabled {
            return;
        }

        sprite_queue.draw_sprite(&self.container_sprite);

        // update animation
        let state = self.resolve_state();

        if self.animator.current_state() != Some(state) {
            self.animator.set_state(state);
            self.animator.set_loop_mode(AnimatorLoopMode::Loop);
        }

        self.animator.apply(&mut self.bar_sprite);

        // multiple calculations below uses this
        let mut frame = self.bar_sprite.frame();

        // position sprite
        let position = self.container_sprite.position() - Vec2::new(frame.width * 0.5, 0.0);
        self.bar_sprite.set_position(position);

        // adjust the frame
        frame.width *= self.progress();
        self.bar_sprite.set_frame(frame);

        sprite_queue.draw_sprite(&self.bar_sprite);
    }
}
