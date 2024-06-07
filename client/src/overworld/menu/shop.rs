use super::Menu;
use crate::overworld::OverworldArea;
use crate::render::ui::{
    FontName, ScrollTracker, TextStyle, Textbox, TextboxDoorstop, TextboxDoorstopRemover,
    TextboxQuestion, UiInputTracker,
};
use crate::render::{Animator, AnimatorLoopMode, Background, FrameTime, SpriteColorQueue};
use crate::resources::{
    AssetManager, Globals, Input, ResourcePaths, TEXT_TRANSPARENT_SHADOW_COLOR,
};
use framework::prelude::*;
use packets::structures::{ShopItem, TextureAnimPathPair};
use std::cell::Cell;
use std::rc::Rc;

// todo:
// const TRANSITION_DURATION: FrameTime = 10;
const LEAVE_DURATION: FrameTime = 30;

pub struct Shop {
    background: Background,
    time: FrameTime,
    confirming_leave: bool,
    leaving: Rc<Cell<bool>>,
    closed: bool,
    close_time: Option<FrameTime>,
    saved_message: String,
    animator: Animator,
    list_sprite: Sprite,
    money_sprite: Sprite,
    recycled_sprite: Sprite,
    list_left: Vec2,
    list_right: Vec2,
    money_amount_right: Vec2,
    idle_cursor: bool,
    scroll_tracker: ScrollTracker,
    ui_input_tracker: UiInputTracker,
    items: Vec<ShopItem>,
    money_text: String,
    base_textbox: Textbox,
    doorstop_remover: Option<TextboxDoorstopRemover>,
    on_select: Box<dyn Fn(&str)>,
    on_description_request: Box<dyn Fn(&str)>,
    on_leave: Rc<dyn Fn()>,
    on_close: Box<dyn Fn()>,
}

impl Shop {
    pub fn new(
        game_io: &GameIO,
        on_select: impl Fn(&str) + 'static,
        on_description_request: impl Fn(&str) + 'static,
        on_leave: impl Fn() + 'static,
        on_close: impl Fn() + 'static,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::OVERWORLD_SHOP_ANIMATION);
        let mut list_sprite = assets.new_sprite(game_io, ResourcePaths::OVERWORLD_SHOP);

        // list
        animator.set_state("LIST");
        animator.apply(&mut list_sprite);

        let list_left = animator.point("LIST_LEFT").unwrap_or_default() - animator.origin();
        let list_right = animator.point("LIST_RIGHT").unwrap_or_default() - animator.origin();

        // cursor
        let mut scroll_tracker = ScrollTracker::new(game_io, 5).with_custom_cursor(
            game_io,
            ResourcePaths::TEXTBOX_CURSOR_ANIMATION,
            ResourcePaths::TEXTBOX_CURSOR,
        );

        let cursor_start = animator.point("CURSOR").unwrap_or_default() - animator.origin();
        scroll_tracker.define_cursor(cursor_start, 16.0);

        // money
        let money_sprite = list_sprite.clone();

        animator.set_state("MONEY");
        animator.apply(&mut list_sprite);
        let money_amount_right =
            animator.point("AMOUNT_RIGHT").unwrap_or_default() - animator.origin();

        // recycled sprite
        let recycled_sprite = money_sprite.clone();

        Self {
            background: Background::new(
                Animator::load_new(assets, ResourcePaths::OVERWORLD_SHOP_BG_ANIMATION),
                assets.new_sprite(game_io, ResourcePaths::OVERWORLD_SHOP_BG),
            ),
            time: 0,
            confirming_leave: false,
            leaving: Rc::new(Cell::new(false)),
            closed: false,
            close_time: None,
            saved_message: String::new(),
            animator,
            list_sprite,
            money_sprite,
            recycled_sprite,
            list_left,
            list_right,
            money_amount_right,
            idle_cursor: false,
            scroll_tracker,
            ui_input_tracker: UiInputTracker::new(),
            items: Vec::new(),
            money_text: String::from("0"),
            base_textbox: Textbox::new_overworld(game_io)
                .with_transition_animation_enabled(false)
                .with_text_animation_enabled(false)
                .begin_open(),
            doorstop_remover: None,
            on_select: Box::new(on_select),
            on_description_request: Box::new(on_description_request),
            on_leave: Rc::new(on_leave),
            on_close: Box::new(on_close),
        }
    }

    pub fn set_shop_avatar(
        &mut self,
        game_io: &GameIO,
        assets: &impl AssetManager,
        path_pair: Option<&TextureAnimPathPair>,
    ) {
        self.base_textbox
            .set_next_avatar(game_io, assets, path_pair);
    }

    pub fn set_message(&mut self, message: String) {
        self.saved_message.clone_from(&message);

        if self.confirming_leave {
            return;
        }

        if let Some(remover) = self.doorstop_remover.take() {
            remover();
        }

        let (interface, remover) = TextboxDoorstop::new();
        self.doorstop_remover = Some(remover);

        self.base_textbox
            .push_interface(interface.with_string(message));
    }

    pub fn set_money(&mut self, money: u32) {
        self.money_text = money.to_string();
    }

    pub fn set_items(&mut self, items: Vec<ShopItem>) {
        self.scroll_tracker.set_total_items(items.len());
        self.items = items;
    }

    pub fn update_item(&mut self, item: ShopItem) {
        let item_id = item.id();

        if let Some(stored) = self.items.iter_mut().find(|s| s.id() == item_id) {
            *stored = item;
        } else {
            log::warn!("Server attempted to update missing shop item: {item_id:?}");
        }
    }

    pub fn remove_item(&mut self, id: &str) {
        if let Some(index) = self.items.iter().position(|s| s.id() == id) {
            self.items.remove(index);
            self.scroll_tracker.set_total_items(self.items.len());
        }
    }

    pub fn close(&mut self) {
        if !self.closed {
            self.closed = true;
            (self.on_close)();
        }
    }
}

impl Menu for Shop {
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

    fn update(&mut self, game_io: &mut GameIO, _area: &mut OverworldArea) {
        self.ui_input_tracker.update(game_io);
        self.time += 1;
        self.background.update();
        self.base_textbox.update(game_io);

        if !self.closed
            && matches!(&self.close_time, Some(time) if self.time >= *time + LEAVE_DURATION)
        {
            self.close();
        }
    }

    fn handle_input(&mut self, game_io: &mut GameIO, _: &mut OverworldArea, textbox: &mut Textbox) {
        if self.closed || self.close_time.is_some() {
            return;
        }

        if self.confirming_leave {
            self.confirming_leave = false;
            self.set_message(self.saved_message.clone());

            // provide a one frame break
            // prevents Input::Cancel on the confirmation question from reactivating the question
            return;
        }

        if self.leaving.get() {
            self.close_time = Some(self.time);
            return;
        }

        self.idle_cursor = false;

        let globals = game_io.resource::<Globals>().unwrap();
        let audio = &globals.audio;

        let previous_index = self.scroll_tracker.selected_index();

        if self.ui_input_tracker.is_active(Input::Up) {
            self.scroll_tracker.move_up();
        }

        if self.ui_input_tracker.is_active(Input::Down) {
            self.scroll_tracker.move_down();
        }

        let selected_index = self.scroll_tracker.selected_index();

        if selected_index != previous_index {
            audio.play_sound(&globals.sfx.cursor_move);
        }

        if self.ui_input_tracker.is_active(Input::Confirm) {
            if let Some(item) = self.items.get(selected_index) {
                (self.on_select)(item.id());
                audio.play_sound(&globals.sfx.cursor_select);
                return;
            }
        }

        if self.ui_input_tracker.is_active(Input::Info) {
            if let Some(item) = self.items.get(selected_index) {
                (self.on_description_request)(item.id());
                audio.play_sound(&globals.sfx.cursor_move);
                return;
            }
        }

        if self.ui_input_tracker.is_active(Input::Cancel) {
            audio.play_sound(&globals.sfx.cursor_cancel);

            let on_leave = self.on_leave.clone();
            let leaving = self.leaving.clone();

            let question_interface = TextboxQuestion::new(String::from("Leaving already?"), {
                move |yes| {
                    if yes {
                        on_leave();
                        leaving.set(true);
                    }
                }
            });

            textbox.use_player_avatar(game_io);
            textbox.push_interface(question_interface);
            textbox.open();

            // hide the base text until
            let saved_message = std::mem::take(&mut self.saved_message);
            self.set_message(String::new());
            self.confirming_leave = true;
            self.saved_message = saved_message;
        }
    }

    fn draw(
        &mut self,
        game_io: &GameIO,
        render_pass: &mut RenderPass,
        sprite_queue: &mut SpriteColorQueue,
        _: &OverworldArea,
    ) {
        self.background.draw(game_io, render_pass);

        // draw static sprites
        sprite_queue.draw_sprite(&self.list_sprite);
        sprite_queue.draw_sprite(&self.money_sprite);

        // draw items
        let mut text_style = TextStyle::new(game_io, FontName::Thin);
        text_style.color = Color::new(0.22, 0.45, 0.55, 1.0);
        text_style.shadow_color = TEXT_TRANSPARENT_SHADOW_COLOR;

        let mut offset = Vec2::ZERO;

        for item in &self.items[self.scroll_tracker.view_range()] {
            // draw name
            let name_position = self.list_left + offset;
            text_style.bounds.set_position(name_position);
            text_style.draw(game_io, sprite_queue, &item.name);

            // draw price
            let text_offset = Vec2::new(-text_style.measure(&item.price_text).size.x, 0.0);
            let price_position = self.list_right + offset + text_offset;

            text_style.bounds.set_position(price_position);
            text_style.draw(game_io, sprite_queue, &item.price_text);

            offset.y += 16.0;
        }

        // draw cursor
        self.scroll_tracker.set_idle(self.idle_cursor);
        self.scroll_tracker.draw_cursor(sprite_queue);
        self.idle_cursor = true;

        // draw arrows
        if self.scroll_tracker.top_index() != 0 {
            self.animator.set_state("ARROW_UP");
            self.animator.set_loop_mode(AnimatorLoopMode::Loop);
            self.animator.sync_time(self.time);
            self.animator.apply(&mut self.recycled_sprite);
            sprite_queue.draw_sprite(&self.recycled_sprite);
        }

        let total_items = self.scroll_tracker.total_items();

        if self.scroll_tracker.view_range().end != total_items {
            self.animator.set_state("ARROW_DOWN");
            self.animator.set_loop_mode(AnimatorLoopMode::Loop);
            self.animator.sync_time(self.time);
            self.animator.apply(&mut self.recycled_sprite);
            sprite_queue.draw_sprite(&self.recycled_sprite);
        }

        // draw money
        let money_offset = Vec2::new(-text_style.measure(&self.money_text).size.x, 0.0);
        let money_position = self.money_amount_right + money_offset;
        text_style.bounds.set_position(money_position);
        text_style.draw(game_io, sprite_queue, &self.money_text);

        // draw textbox
        self.base_textbox.draw(game_io, sprite_queue);
    }
}
