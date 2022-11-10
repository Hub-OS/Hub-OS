use crate::render::*;
use crate::resources::{AssetManager, Globals, ResourcePaths, RESOLUTION_F};
use framework::prelude::{GameIO, Sprite, Vec2};

#[derive(Clone)]
pub struct TurnGauge {
    time: FrameTime,
    max_time: FrameTime,
    animator: Animator,
    container_sprite: Sprite,
    sprite: Sprite,
}

impl TurnGauge {
    pub const DEFAULT_MAX_TIME: FrameTime = 512;

    pub fn new(game_io: &GameIO<Globals>) -> Self {
        let assets = &game_io.globals().assets;

        let sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_TURN_GAUGE);
        let mut container_sprite = sprite.clone();
        container_sprite.set_position(Vec2::new(RESOLUTION_F.x * 0.5, 0.0));

        let mut animator = Animator::load_new(assets, ResourcePaths::BATTLE_TURN_GAUGE_ANIMATION);
        animator.set_state("CONTAINER");
        animator.apply(&mut container_sprite);

        Self {
            time: 0,
            max_time: Self::DEFAULT_MAX_TIME,
            animator,
            sprite,
            container_sprite,
        }
    }

    pub fn increment_time(&mut self) {
        self.time = self.max_time.min(self.time + 1);
    }

    pub fn is_complete(&self) -> bool {
        self.time >= self.max_time
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

    fn resolve_state(&self) -> &'static str {
        if self.is_complete() {
            "READY"
        } else {
            "STRETCH"
        }
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue) {
        sprite_queue.draw_sprite(&self.container_sprite);

        // update animation
        let state = self.resolve_state();

        if self.animator.current_state() != Some(state) {
            self.animator.set_state(state);
            self.animator.set_loop_mode(AnimatorLoopMode::Loop);
        } else {
            self.animator.update();
        }

        self.animator.apply(&mut self.sprite);

        // multiple calculations below uses this
        let mut frame = self.sprite.frame();

        // position sprite
        let position = self.container_sprite.position() - Vec2::new(frame.width * 0.5, 0.0);
        self.sprite.set_position(position);

        // adjust the frame
        frame.width *= self.progress();
        self.sprite.set_frame(frame);

        sprite_queue.draw_sprite(&self.sprite);
    }
}
