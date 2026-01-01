use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

pub struct ContextFrame {
    label: String,
    label_size: Vec2,
    label_9patch: NinePatch,
    body_9patch: NinePatch,
    arrow_animator: Animator,
    animation_time: FrameTime,
    inner_size: Vec2,
    outer_size: Vec2,
    center: Vec2,
}

impl ContextFrame {
    pub fn new(game_io: &GameIO, label: String) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // resources
        let texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);

        // styles
        let label_9patch = build_9patch!(game_io, texture.clone(), &animator, "CONTEXT_LABEL");
        let body_9patch = build_9patch!(game_io, texture, &animator, "CONTEXT_BODY");

        let label_size = TextStyle::new(game_io, FontName::Context)
            .measure(&label)
            .size;

        Self {
            label,
            label_size,
            label_9patch,
            body_9patch,
            arrow_animator: Animator::load_new(assets, ResourcePaths::MORE_ARROWS_ANIMATION),
            animation_time: 0,
            inner_size: Vec2::ZERO,
            outer_size: Vec2::ZERO,
            center: RESOLUTION_F * 0.5,
        }
    }

    pub fn with_inner_size(mut self, size: Vec2) -> Self {
        self.inner_size = size;
        self.resolve_outer_size();
        self
    }

    pub fn with_center(mut self, position: Vec2) -> Self {
        self.center = position;
        self
    }

    pub fn set_inner_size(&mut self, size: Vec2) {
        self.inner_size = size;
        self.resolve_outer_size();
    }

    pub fn set_center(&mut self, position: Vec2) {
        self.center = position;
    }

    pub fn inner_top_left(&self) -> Vec2 {
        let mut top_left = self.center - self.outer_size * 0.5;

        top_left.y += self.label_9patch.heights() + self.label_size.y;

        top_left.x += self.body_9patch.left_width();
        top_left.y += self.body_9patch.top_height();

        top_left
    }

    pub fn inner_size(&self) -> Vec2 {
        self.inner_size
    }

    fn resolve_outer_size(&mut self) {
        // apply inner size
        self.outer_size = self.inner_size;

        // apply label
        self.outer_size.x = self.outer_size.x.max(self.label_size.x);
        self.outer_size.y += self.label_size.y;

        // apply frame
        self.outer_size.y += self.label_9patch.heights() + self.body_9patch.heights();
        self.outer_size.x += self.label_9patch.widths().max(self.body_9patch.widths());
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let outer_top_left = self.center - self.outer_size * 0.5;

        // render label
        let label_border_top = self.label_9patch.top_height();
        let label_border_bottom = self.label_9patch.bottom_height();
        let label_border_heights = label_border_top + label_border_bottom;

        let label_top_left = outer_top_left
            + Vec2::new(
                self.label_9patch.left_width(),
                self.label_9patch.top_height(),
            );

        let mut text_style = TextStyle::new(game_io, FontName::Context);
        text_style.bounds.set_position(label_top_left);

        let label_size = Vec2::new(
            self.outer_size.x - self.label_9patch.widths(),
            text_style.line_height(),
        );

        self.label_9patch.draw(
            sprite_queue,
            Rect::from_corners(label_top_left, label_top_left + label_size),
        );

        text_style.font = FontName::Context;
        text_style.draw(game_io, sprite_queue, &self.label);

        // render body
        let mut body_top_left = outer_top_left;
        body_top_left.x += self.body_9patch.left_width();
        body_top_left.y +=
            label_border_heights + text_style.line_height() + self.body_9patch.top_height();

        let mut body_bottom_right = self.center + self.outer_size * 0.5;
        body_bottom_right.x -= self.body_9patch.right_width();
        body_bottom_right.y -= self.body_9patch.bottom_height();

        self.body_9patch.draw(
            sprite_queue,
            Rect::from_corners(body_top_left, body_bottom_right),
        );
    }

    pub fn draw_scroll_arrows(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        scroll_tracker: &ScrollTracker,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut arrow_sprite = assets.new_sprite(game_io, ResourcePaths::MORE_ARROWS);

        let mut draw_state = |sprite: &mut Sprite, state: &str| {
            self.arrow_animator.set_state(state);
            self.arrow_animator.set_loop_mode(AnimatorLoopMode::Loop);
            self.arrow_animator.sync_time(self.animation_time);
            self.arrow_animator.apply(sprite);
            sprite_queue.draw_sprite(sprite);
        };

        let view_range = scroll_tracker.view_range();

        if view_range.start > 0 {
            // snap to corner
            let mut arrow_position = self.center;
            arrow_position.x += self.outer_size.x * 0.5;
            arrow_position.y -= self.outer_size.y * 0.5;

            // account for borders
            arrow_position.x -= self.body_9patch.right_width();
            arrow_position.y += self.label_9patch.heights()
                + TextStyle::new(game_io, FontName::Context).line_height();

            arrow_sprite.set_position(arrow_position);
            draw_state(&mut arrow_sprite, "ARROW_UP");
        }

        if view_range.end < scroll_tracker.total_items() {
            // snap to corner
            let mut arrow_position = self.center + self.outer_size * 0.5;

            // account for borders
            arrow_position.x -= self.body_9patch.right_width();
            arrow_position.y -= self.body_9patch.bottom_height() - 2.0;

            arrow_sprite.set_position(arrow_position);
            draw_state(&mut arrow_sprite, "ARROW_DOWN");
        }

        self.animation_time += 1;
    }
}
