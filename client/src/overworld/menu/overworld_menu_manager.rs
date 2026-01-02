use super::{Bbs, ItemsMenu, Shop};
use crate::ease::inverse_lerp;
use crate::overworld::components::WidgetAttachment;
use crate::overworld::{OverworldArea, OverworldEvent, OverworldPlayerData};
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::scenes::KeyItemsScene;
use crate::structures::{GenerationalIndex, SlotMap};
use crate::transitions::DEFAULT_FADE_DURATION;
use framework::prelude::*;
use packets::structures::TextureAnimPathPair;

enum Event {
    SelectionMade,
    SelectionAck,
    ShopOrBbsReplaced,
    ShopOrBbsClosed { transition: bool },
}

pub trait Menu {
    fn drop_on_close(&self) -> bool {
        false
    }

    fn is_fullscreen(&self) -> bool;
    fn is_open(&self) -> bool;
    fn open(&mut self, game_io: &mut GameIO, area: &mut OverworldArea);
    fn update(&mut self, game_io: &mut GameIO, area: &mut OverworldArea);
    fn handle_input(
        &mut self,
        game_io: &mut GameIO,
        area: &mut OverworldArea,
        textbox: &mut Textbox,
    );
    fn draw(
        &mut self,
        game_io: &GameIO,
        render_pass: &mut RenderPass,
        sprite_queue: &mut SpriteColorQueue,
        area: &OverworldArea,
    );
}

pub struct OverworldMenuManager {
    selection_needs_ack: bool,
    textbox: Textbox,
    shop_or_bbs_replaced: bool,
    bbs: Option<Bbs>,
    shop: Option<Shop>,
    fade_time: FrameTime,
    max_fade_time: FrameTime,
    fade_sprite: Sprite,
    navigation_menu: NavigationMenu,
    menus: SlotMap<Box<dyn Menu>>,
    menu_bindings: Vec<(Input, GenerationalIndex)>,
    active_menu: Option<GenerationalIndex>,
    fading_menu: Option<GenerationalIndex>,
    event_receiver: flume::Receiver<Event>,
    event_sender: flume::Sender<Event>,
}

impl OverworldMenuManager {
    pub fn new(game_io: &GameIO) -> Self {
        let assets = &Globals::from_resources(game_io).assets;
        let mut fade_sprite = assets.new_sprite(game_io, ResourcePaths::PIXEL);
        fade_sprite.set_color(Color::BLACK);
        fade_sprite.set_size(RESOLUTION_F);

        let max_fade_time =
            (DEFAULT_FADE_DURATION.as_secs_f32() * game_io.target_fps() as f32) as FrameTime;

        let (event_sender, event_receiver) = flume::unbounded();

        let navigation_menu = NavigationMenu::new(
            game_io,
            vec![
                SceneOption::Decks,
                SceneOption::Items,
                SceneOption::Library,
                SceneOption::Character,
                // SceneOption::Email,
                SceneOption::KeyItems,
                SceneOption::BattleSelect,
                SceneOption::Config,
            ],
        )
        .into_overlay(true);

        Self {
            selection_needs_ack: false,
            textbox: Textbox::new_overworld(game_io),
            shop_or_bbs_replaced: false,
            bbs: None,
            shop: None,
            fade_time: max_fade_time,
            max_fade_time,
            fade_sprite,
            navigation_menu,
            menus: Default::default(),
            menu_bindings: Vec::new(),
            active_menu: None,
            fading_menu: None,
            event_sender,
            event_receiver,
        }
    }

    pub fn is_open(&self) -> bool {
        self.active_menu.is_some()
            || self.textbox.is_open()
            || self.bbs.is_some()
            || self.shop.is_some()
            || self.navigation_menu.is_open()
            || self.fading_menu.is_some()
    }

    pub fn is_textbox_open(&self) -> bool {
        self.textbox.is_open()
    }

    pub fn is_blocking_hud(&self) -> bool {
        self.navigation_menu.is_open() || self.is_blocking_view()
    }

    pub fn is_blocking_view(&self) -> bool {
        let fading_out = self.fade_time <= self.max_fade_time / 2;

        let fading_menu_blocks =
            matches!(&self.fading_menu, Some(index) if self.menus[*index].is_fullscreen());

        let menu_is_fullscreen = matches!(&self.active_menu, Some(index) if self.menus[*index].is_fullscreen())
            || self.shop.is_some()
            || self.bbs.is_some();

        (!fading_out && menu_is_fullscreen) || (fading_out && fading_menu_blocks)
    }

    pub fn bbs_mut(&mut self) -> Option<&mut Bbs> {
        self.bbs.as_mut()
    }

    pub fn shop_mut(&mut self) -> Option<&mut Shop> {
        self.shop.as_mut()
    }

    pub fn register_menu(&mut self, menu: Box<dyn Menu>) -> GenerationalIndex {
        self.menus.insert(menu)
    }

    /// Binds an input event to open a menu, avoids opening menus while other menus are open
    pub fn bind_menu(&mut self, input: Input, menu_index: GenerationalIndex) {
        self.menu_bindings.push((input, menu_index));
    }

    /// Opens a registered menu, forces other menus to close
    pub fn open_menu(
        &mut self,
        game_io: &mut GameIO,
        area: &mut OverworldArea,
        menu_index: GenerationalIndex,
    ) {
        let menu = &mut self.menus[menu_index];
        menu.open(game_io, area);

        let next_is_fullscreen = menu.is_fullscreen();
        let previous_is_fullscreen =
            matches!(self.active_menu, Some(index) if self.menus[index].is_fullscreen());

        self.close_old_menu(next_is_fullscreen || previous_is_fullscreen);
        self.active_menu = Some(menu_index);
    }

    pub fn acknowledge_selection(&mut self) {
        // sending an event in case the SelectionMade event is still queued
        self.event_sender.send(Event::SelectionAck).unwrap();
    }

    // todo: fix argument count
    #[allow(clippy::too_many_arguments)]
    pub fn open_bbs(
        &mut self,
        game_io: &GameIO,
        topic: String,
        color: Color,
        transition: bool,
        on_select: impl Fn(&str) + 'static,
        request_more: impl Fn() + 'static,
        on_close: impl Fn() + 'static,
    ) {
        self.close_old_menu(transition);

        let on_select = {
            let event_sender = self.event_sender.clone();

            move |id: &str| {
                event_sender.send(Event::SelectionMade).unwrap();
                on_select(id);
            }
        };

        let on_close = {
            let event_sender = self.event_sender.clone();

            move || {
                event_sender
                    .send(Event::ShopOrBbsClosed { transition })
                    .unwrap();
                on_close();
            }
        };

        self.bbs = Some(Bbs::new(
            game_io,
            topic,
            color,
            on_select,
            request_more,
            on_close,
        ));
    }

    pub fn open_shop(
        &mut self,
        game_io: &GameIO,
        on_select: impl Fn(&str) + 'static,
        on_description_request: impl Fn(&str) + 'static,
        on_leave: impl Fn() + 'static,
        on_close: impl Fn() + 'static,
    ) {
        self.close_old_menu(true);

        let on_select = {
            let event_sender = self.event_sender.clone();

            move |id: &str| {
                event_sender.send(Event::SelectionMade).unwrap();
                on_select(id);
            }
        };

        let on_description_request = {
            let event_sender = self.event_sender.clone();

            move |id: &str| {
                event_sender.send(Event::SelectionMade).unwrap();
                on_description_request(id);
            }
        };

        let on_leave = {
            let event_sender = self.event_sender.clone();

            move || {
                event_sender.send(Event::SelectionMade).unwrap();
                on_leave();
            }
        };

        let on_close = {
            let event_sender = self.event_sender.clone();

            move || {
                event_sender
                    .send(Event::ShopOrBbsClosed { transition: true })
                    .unwrap();
                on_close();
            }
        };

        self.shop = Some(Shop::new(
            game_io,
            on_select,
            on_description_request,
            on_leave,
            on_close,
        ));
    }

    // call before setting shop or bbs
    fn close_old_menu(&mut self, transition: bool) {
        if let Some(bbs) = &mut self.bbs {
            self.event_sender.send(Event::ShopOrBbsReplaced).unwrap();
            bbs.close();
        }

        if let Some(shop) = &mut self.shop {
            self.event_sender.send(Event::ShopOrBbsReplaced).unwrap();
            shop.close();
        }

        if transition {
            self.fading_menu = self.active_menu.take();
            self.fade_time = 0;

            if let Some(menu) = self.shop.take() {
                self.create_fading_menu(menu);
            }

            if let Some(menu) = self.bbs.take() {
                self.create_fading_menu(menu);
            }
        } else {
            self.delete_fading_menu();
            self.shop = None;
            self.bbs = None;
            self.active_menu = None;
        }
    }

    fn create_fading_menu(&mut self, menu: impl Menu + 'static) {
        self.delete_fading_menu();

        let index = self.register_menu(Box::new(menu));
        self.fading_menu = Some(index);
        self.fade_time = 0;
    }

    fn delete_fading_menu(&mut self) {
        let Some(index) = self.fading_menu.take() else {
            return;
        };

        if self.menus[index].drop_on_close() {
            self.menus.remove(index);
        }
    }

    pub fn set_next_avatar(
        &mut self,
        game_io: &GameIO,
        assets: &impl AssetManager,
        path_pair: Option<&TextureAnimPathPair>,
    ) {
        self.textbox.set_next_avatar(game_io, assets, path_pair);
    }

    pub fn set_last_text_style(&mut self, text_style: TextStyle) {
        self.textbox.set_last_text_style(text_style);
    }

    pub fn use_player_avatar(&mut self, game_io: &GameIO) {
        self.textbox.use_navigation_avatar(game_io);
    }

    pub fn remaining_textbox_interfaces(&self) -> usize {
        self.textbox.remaining_interfaces()
    }

    pub fn push_textbox_interface(&mut self, interface: impl TextboxInterface + 'static) {
        self.textbox.push_interface(interface);
        self.textbox.open();
    }

    pub fn update_player_data(&mut self, player_data: &OverworldPlayerData) {
        self.navigation_menu.update_info(player_data);

        if let Some(shop) = self.shop_mut() {
            shop.set_money(player_data.money);
        }
    }

    fn handle_events(&mut self) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::SelectionMade => {
                    self.selection_needs_ack = true;
                }
                Event::SelectionAck => {
                    self.selection_needs_ack = false;
                }
                Event::ShopOrBbsReplaced => {
                    self.shop_or_bbs_replaced = true;
                }
                Event::ShopOrBbsClosed { transition } => {
                    if !self.shop_or_bbs_replaced {
                        // if the shop or bbs was replaced transition will be handled by the replacer
                        // otherwise we must handle it here

                        if transition {
                            if let Some(menu) = self.shop.take() {
                                self.create_fading_menu(menu);
                            }

                            if let Some(menu) = self.bbs.take() {
                                self.create_fading_menu(menu);
                            }
                        } else {
                            self.bbs = None;
                            self.shop = None;
                            self.delete_fading_menu();
                        }
                    }

                    self.shop_or_bbs_replaced = false;
                }
            }
        }
    }

    fn update_navigation_menu(
        &mut self,
        game_io: &mut GameIO,
        area: &mut OverworldArea,
    ) -> NextScene {
        let mut navigation_selection = None;

        let next_scene =
            self.navigation_menu
                .update(game_io, |game_io, selection| match selection {
                    SceneOption::KeyItems => Some(Box::new(KeyItemsScene::new(
                        game_io,
                        &area.item_registry,
                        &area.player_data.inventory,
                    ))),
                    _ => {
                        navigation_selection = Some(selection);
                        None
                    }
                });

        if let Some(SceneOption::Items) = navigation_selection {
            let menu_event_sender = self.event_sender.clone();
            let area_event_sender = area.event_sender.clone();

            let on_select = move |id: &str| {
                menu_event_sender.send(Event::SelectionMade).unwrap();

                let overworld_event = OverworldEvent::ItemUse(id.to_string());
                area_event_sender.send(overworld_event).unwrap();
            };

            let menu_index = self.register_menu(Box::new(ItemsMenu::new(game_io, area, on_select)));
            self.open_menu(game_io, area, menu_index);
        }

        next_scene
    }

    pub fn update(&mut self, game_io: &mut GameIO, area: &mut OverworldArea) -> NextScene {
        let textbox_animations_enabled = !self.shop.is_some();
        self.textbox
            .set_transition_animation_enabled(textbox_animations_enabled);
        self.textbox
            .set_text_animation_enabled(textbox_animations_enabled);

        if self.fade_time < self.max_fade_time {
            self.fade_time += 1;

            let fade_progress = inverse_lerp!(0, self.max_fade_time, self.fade_time);

            if fade_progress > 0.5 {
                // clear out the previous bbs to avoid extra updates
                self.delete_fading_menu();
            }
        }

        // handle events created outside of this update
        self.handle_events();

        let menus_block_view = self.is_blocking_view();
        let in_transition = game_io.is_in_transition() || self.fade_time != self.max_fade_time;
        let mut handle_input =
            (menus_block_view || !self.navigation_menu.is_open()) && !in_transition;

        // update textbox
        if self.textbox.is_complete() {
            self.textbox.close();
        }

        if handle_input {
            self.textbox.update(game_io);
        }

        // skip other input checks if a textbox is open
        handle_input &= !self.textbox.is_open();

        // skip other input checks if we're waiting for an ack
        handle_input &= !self.selection_needs_ack;

        // update the active registered menu
        if let Some(index) = self.active_menu {
            let menu = &mut self.menus[index];

            if handle_input {
                menu.handle_input(game_io, area, &mut self.textbox);
            }

            menu.update(game_io, area);

            // skip other input checks while a registered menu is open
            handle_input = false;

            if !menu.is_open() {
                let transition = menu.is_fullscreen();
                self.close_old_menu(transition);
            }
        }

        // update menus fading away
        if let Some(index) = self.fading_menu {
            self.menus[index].update(game_io, area);
        }

        // update bbs
        if let Some(bbs) = &mut self.bbs {
            if handle_input {
                bbs.handle_input(game_io, area, &mut self.textbox);
            }

            bbs.update(game_io, area);

            // skip other input checks while a bbs is open
            handle_input = false;
        }

        // update shop
        if let Some(shop) = &mut self.shop {
            if handle_input {
                shop.handle_input(game_io, area, &mut self.textbox);
            }

            shop.update(game_io, area);

            // skip other input checks while a shop is open
            handle_input = false;
        }

        // try opening a menu if there's no menu open
        if handle_input && !self.is_open() {
            let input_util = InputUtil::new(game_io);

            for (input, index) in self.menu_bindings.iter().cloned() {
                if input_util.was_just_pressed(input) {
                    self.open_menu(game_io, area, index);

                    // skip other input checks while a menu is opening
                    handle_input = false;
                    break;
                }
            }

            let input_util = InputUtil::new(game_io);

            if handle_input && input_util.was_just_pressed(Input::Pause) {
                let globals = Globals::from_resources(game_io);
                globals.audio.play_sound(&globals.sfx.card_select_open);
                self.navigation_menu.open();
            }
        }

        let mut next_scene = NextScene::None;

        if !menus_block_view && !in_transition {
            // only update the navigation menu if menus or transitions aren't blocking it
            next_scene = self.update_navigation_menu(game_io, area);
        }

        // handle events created during this update
        self.handle_events();

        next_scene
    }

    pub fn draw(
        &mut self,
        game_io: &GameIO,
        render_pass: &mut RenderPass,
        sprite_queue: &mut SpriteColorQueue,
        area: &OverworldArea,
    ) {
        // draw names
        if !self.is_blocking_hud() {
            area.draw_player_names_at(game_io, sprite_queue, game_io.input().mouse_position());

            for touch in game_io.input().touches() {
                let mut render_point = touch.position;

                // shift to avoid rendering directly under the finger
                render_point.y += 0.15;

                area.draw_player_names_at(game_io, sprite_queue, render_point);
            }
        }

        let fade_progress = inverse_lerp!(0, self.max_fade_time, self.fade_time);

        // draw menus
        if fade_progress < 0.5 {
            if let Some(index) = self.fading_menu {
                let menu = &mut self.menus[index];
                menu.draw(game_io, render_pass, sprite_queue, area);
            }
        } else {
            if let Some(bbs) = &mut self.bbs {
                bbs.draw(game_io, render_pass, sprite_queue, area);
            }

            if let Some(shop) = &mut self.shop {
                shop.draw(game_io, render_pass, sprite_queue, area);
            }

            if let Some(index) = self.active_menu {
                self.menus[index].draw(game_io, render_pass, sprite_queue, area);
            }
        }

        // draw textbox
        self.textbox.draw(game_io, sprite_queue);

        // draw widget attachments
        area.draw_screen_attachments::<WidgetAttachment>(game_io, sprite_queue);

        if !self.is_blocking_view() {
            // draw navigation menu
            self.navigation_menu.draw(game_io, sprite_queue);
        }

        // draw fade
        if self.fade_time < self.max_fade_time {
            let mut color = self.fade_sprite.color();
            color.a = crate::ease::symmetric(4.0, fade_progress);
            self.fade_sprite.set_color(color);

            sprite_queue.draw_sprite(&self.fade_sprite);
        }
    }
}
