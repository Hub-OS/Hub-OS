use super::Menu;
use crate::overworld::OverworldArea;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use packets::structures::BbsPost;
use std::iter::IntoIterator;

pub struct Bbs {
    topic: String,
    color: Color,
    ui_input_tracker: UiInputTracker,
    scroll_tracker: ScrollTracker,
    reached_end: bool,
    posts: Vec<BbsPost>,
    list_point: Vec2,
    unread_sprite: Sprite,
    unread_animator: Animator,
    static_sprites: [Sprite; 3],
    on_select: Box<dyn Fn(&str)>,
    request_more: Box<dyn Fn()>,
    on_close: Option<Box<dyn Fn()>>,
}

impl Bbs {
    pub fn new(
        game_io: &GameIO,
        topic: String,
        color: Color,
        on_select: impl Fn(&str) + 'static,
        request_more: impl Fn() + 'static,
        on_close: impl Fn() + 'static,
    ) -> Self {
        let mut scroll_tracker = ScrollTracker::new(game_io, 8);
        let assets = &Globals::from_resources(game_io).assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::OVERWORLD_BBS_ANIMATION);
        let mut bg_sprite = assets.new_sprite(game_io, ResourcePaths::OVERWORLD_BBS);
        let mut frame_sprite = bg_sprite.clone();
        let mut posts_bg_sprite = bg_sprite.clone();

        // colors
        bg_sprite.set_color(color);
        frame_sprite.set_color(Color::lerp(Color::WHITE, color, 0.025));
        posts_bg_sprite.set_color(Color::lerp(Color::WHITE, color, 0.5));

        // bg
        animator.set_state("BG");
        animator.apply(&mut bg_sprite);

        // frame
        let frame_point = animator.point_or_zero("FRAME");
        frame_sprite.set_position(frame_point);

        animator.set_state("FRAME");
        animator.apply(&mut frame_sprite);

        // scrollbar
        let scroll_start = animator.point_or_zero("SCROLL_START") + frame_point;
        let scroll_end = animator.point_or_zero("SCROLL_END") + frame_point;
        scroll_tracker.define_scrollbar(scroll_start, scroll_end);

        // posts_bg
        let posts_bg_point = animator.point_or_zero("POSTS_BG") + frame_point;
        posts_bg_sprite.set_position(posts_bg_point);

        animator.set_state("POSTS_BG");
        animator.apply(&mut posts_bg_sprite);

        // list start + cursor
        let list_point = animator.point_or_zero("POSTS_START") + posts_bg_point;

        let cursor_start = list_point + Vec2::new(-5.0, 3.0);
        scroll_tracker.define_cursor(cursor_start, 16.0);

        Self {
            topic,
            color,
            ui_input_tracker: UiInputTracker::new(),
            scroll_tracker,
            reached_end: false,
            posts: Vec::new(),
            list_point,
            unread_sprite: assets.new_sprite(game_io, ResourcePaths::UNREAD),
            unread_animator: Animator::load_new(assets, ResourcePaths::UNREAD_ANIMATION)
                .with_state("DEFAULT")
                .with_loop_mode(AnimatorLoopMode::Loop),
            static_sprites: [bg_sprite, frame_sprite, posts_bg_sprite],
            on_select: Box::new(on_select),
            request_more: Box::new(request_more),
            on_close: Some(Box::new(on_close)),
        }
    }

    pub fn close(&mut self) {
        if let Some(callback) = self.on_close.take() {
            callback();
        }
    }

    pub fn prepend_posts(
        &mut self,
        before_id: Option<&str>,
        posts: impl IntoIterator<Item = BbsPost>,
    ) {
        let at_index = before_id
            .and_then(|id| self.posts.iter().position(|post| post.id == id))
            .unwrap_or_default();

        self.posts.splice(at_index..at_index, posts);

        // update selection
        let increase = self.posts.len() - self.scroll_tracker.total_items();
        self.scroll_tracker.handle_insert(at_index, increase);
    }

    pub fn append_posts(
        &mut self,
        after_id: Option<&str>,
        posts: impl IntoIterator<Item = BbsPost>,
    ) {
        let reference_index =
            after_id.and_then(|id| self.posts.iter().position(|post| post.id == id));

        if let Some(index) = reference_index {
            let after_index = index + 1;

            if after_index == self.posts.len() {
                // added new posts to the end
                self.reached_end = false;
            }

            // insert posts
            self.posts.splice(after_index..after_index, posts);

            // update selection
            let increase = self.posts.len() - self.scroll_tracker.total_items();
            self.scroll_tracker.handle_insert(after_index, increase);
        } else {
            // add new posts to the end
            self.posts.extend(posts);
            self.reached_end = false;
            self.scroll_tracker.set_total_items(self.posts.len());
        }
    }

    pub fn remove_post(&mut self, id: &str) {
        if let Some(index) = self.posts.iter().position(|post| post.id == id) {
            self.posts.remove(index);
            self.scroll_tracker.handle_remove(index);
        }
    }
}

impl Menu for Bbs {
    fn drop_on_close(&self) -> bool {
        true
    }

    fn is_fullscreen(&self) -> bool {
        true
    }

    fn is_open(&self) -> bool {
        true
    }

    fn open(&mut self, _game_io: &mut GameIO, _area: &mut OverworldArea) {}

    fn update(&mut self, _game_io: &mut GameIO, _area: &mut crate::overworld::OverworldArea) {
        self.unread_animator.update();
    }

    fn handle_input(&mut self, game_io: &mut GameIO, _: &mut OverworldArea, _: &mut Textbox) {
        let globals = Globals::from_resources(game_io);

        self.ui_input_tracker.update(game_io);

        // handle moving cursor
        let old_index = self.scroll_tracker.selected_index();

        self.scroll_tracker
            .handle_vertical_input(&self.ui_input_tracker);

        if old_index != self.scroll_tracker.selected_index() {
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        // if we're within two pages of the end, send signal for more data
        if !self.reached_end
            && self.scroll_tracker.top_index() + self.scroll_tracker.view_size() * 2
                >= self.posts.len()
        {
            self.reached_end = true;
            (self.request_more)();
        }

        // handle selection
        let input_util = InputUtil::new(game_io);

        if !self.posts.is_empty() && input_util.was_just_pressed(Input::Confirm) {
            globals.audio.play_sound(&globals.sfx.cursor_select);

            let post = &mut self.posts[self.scroll_tracker.selected_index()];
            post.read = true;

            (self.on_select)(&post.id);
        } else if input_util.was_just_pressed(Input::Cancel) {
            globals.audio.play_sound(&globals.sfx.cursor_cancel);
            self.close();
        }
    }

    fn draw(
        &mut self,
        game_io: &GameIO,
        _: &mut RenderPass,
        sprite_queue: &mut SpriteColorQueue,
        _: &OverworldArea,
    ) {
        for sprite in &self.static_sprites {
            sprite_queue.draw_sprite(sprite);
        }

        self.unread_animator.apply(&mut self.unread_sprite);

        // draw topic
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.bounds.set_position(Vec2::new(8.0, 3.0));
        text_style.shadow_color = Color::lerp(Color::BLACK, self.color, 0.7);
        text_style.draw(game_io, sprite_queue, &self.topic);

        // draw posts
        text_style.font = FontName::Thin;
        text_style.monospace = true;
        text_style.color = Color::from((74, 65, 74));
        text_style.shadow_color = Color::from((0, 0, 0, 25));
        text_style.bounds.set_position(self.list_point);
        text_style.bounds.y += 3.0;

        for i in self.scroll_tracker.view_range() {
            let post = &self.posts[i];

            if !post.read {
                let mut unread_position = text_style.bounds.position();
                unread_position.x = self.list_point.x + 8.0;
                unread_position.y += self.scroll_tracker.cursor_multiplier() * 0.5 - 3.0;

                self.unread_sprite.set_position(unread_position);
                sprite_queue.draw_sprite(&self.unread_sprite);
            }

            text_style.bounds.x = self.list_point.x + 33.0;
            text_style.draw(game_io, sprite_queue, &post.title);
            text_style.bounds.x = self.list_point.x + 152.0;
            text_style.draw(game_io, sprite_queue, &post.author);

            text_style.bounds.y += self.scroll_tracker.cursor_multiplier();
        }

        // draw cursor + scrollbar
        self.scroll_tracker.draw_cursor(sprite_queue);
        self.scroll_tracker.draw_scrollbar(sprite_queue);
    }
}
