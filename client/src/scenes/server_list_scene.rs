use super::{InitialConnectScene, ServerEditProp, ServerEditScene};
use crate::bindable::SpriteColorMode;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::transitions::*;
use framework::prelude::*;
use packets::{ClientPacket, Reliability, ServerPacket, SERVER_TICK_RATE};

#[derive(PartialEq, Eq, Clone, Copy)]
enum ServerStatus {
    Pending,
    Online,
    Offline,
    TooOld,
    TooNew,
    Incompatible,
    InvalidAddress,
}

impl ServerStatus {
    fn state(self) -> &'static str {
        match self {
            ServerStatus::Pending => "PENDING_BADGE",
            ServerStatus::Online => "ONLINE_BADGE",
            ServerStatus::Offline => "OFFLINE_BADGE",
            ServerStatus::TooOld
            | ServerStatus::TooNew
            | ServerStatus::Incompatible
            | ServerStatus::InvalidAddress => "ERROR_BADGE",
        }
    }

    fn create_message(self, server_name: &str) -> Option<String> {
        let message = match self {
            ServerStatus::Online | ServerStatus::Pending => {
                return None;
            }
            ServerStatus::Offline => format!("{server_name} is offline."),
            ServerStatus::TooOld => {
                format!("{server_name} is too behind. We'll need to downgrade to connect.")
            }
            ServerStatus::TooNew => {
                format!("{server_name} is too ahead. We'll need to update to connect.")
            }
            ServerStatus::Incompatible => format!("{server_name} is incompatible."),
            ServerStatus::InvalidAddress => {
                format!("I couldn't understand the address for {server_name}.")
            }
        };

        Some(message)
    }
}

#[derive(Clone, Copy)]
enum MenuOption {
    New,
    Edit,
    Move,
    Delete,
}

enum Event {
    Delete,
}

pub struct ServerListScene {
    camera: Camera,
    bg_sprite: Sprite,
    animator: Animator,
    list_box_sprite: Sprite,
    list_start: Vec2,
    statuses: Vec<(String, ServerStatus)>, // address, statuses
    scroll_tracker: ScrollTracker,
    ui_input_tracker: UiInputTracker,
    context_menu: ContextMenu<MenuOption>,
    active_poll_task: Option<(String, AsyncTask<ServerStatus>)>,
    textbox: Textbox,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    time: FrameTime,
    next_scene: NextScene<Globals>,
}

impl ServerListScene {
    pub fn new(game_io: &GameIO<Globals>) -> Box<Self> {
        let globals = game_io.globals();
        let assets = &globals.assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::SERVER_LIST_SHEET_ANIMATION);
        let texture = assets.texture(game_io, ResourcePaths::SERVER_LIST_SHEET);

        // bg
        animator.set_state("PAGED_BG");
        let mut bg_sprite = Sprite::new(texture.clone(), globals.default_sampler.clone());
        animator.apply(&mut bg_sprite);

        // list box
        let box_pos = animator.point("LIST_START").unwrap_or_default();
        animator.set_state("LIST_BOX");

        let list_start = animator.point("LIST_START").unwrap_or_default() + box_pos;

        let mut list_box_sprite = Sprite::new(texture, globals.default_sampler.clone());
        list_box_sprite.set_position(box_pos);
        animator.apply(&mut list_box_sprite);

        // scroll tracker
        let scroll_start = animator.point("SCROLL_START").unwrap_or_default() + box_pos;
        let scroll_end = animator.point("SCROLL_END").unwrap_or_default() + box_pos;

        let cursor_start = list_start + Vec2::new(0.0, 5.0);

        let mut scroll_tracker = ScrollTracker::new(game_io, 7);
        scroll_tracker.define_cursor(cursor_start, 16.0);
        scroll_tracker.define_scrollbar(scroll_start, scroll_end);

        // context menu
        let context_position = animator.point("CONTEXT_MENU").unwrap_or_default() + box_pos;
        let context_menu = ContextMenu::new(game_io, "OPTIONS", context_position);

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        Box::new(Self {
            camera: Camera::new_ui(game_io),
            bg_sprite,
            animator,
            list_box_sprite,
            list_start,
            statuses: Vec::new(),
            scroll_tracker,
            ui_input_tracker: UiInputTracker::new(),
            active_poll_task: None,
            context_menu,
            textbox: Textbox::new_navigation(game_io),
            event_sender,
            event_receiver,
            time: 0,
            next_scene: NextScene::None,
        })
    }

    fn update_server_list(&mut self, game_io: &mut GameIO<Globals>) {
        use packets::address_parsing::strip_data;

        let server_list = &game_io.globals().global_save.server_list;

        for (i, server_info) in server_list.iter().enumerate() {
            let new_address = &server_info.address;

            let address = match self.statuses.get(i) {
                Some((address, _)) => address,
                None => {
                    // status list is shorter, just need to append
                    self.statuses
                        .push((new_address.clone(), ServerStatus::Pending));
                    continue;
                }
            };

            if strip_data(address) == strip_data(new_address) {
                continue;
            }

            self.statuses[i] = (new_address.clone(), ServerStatus::Pending);
        }

        self.scroll_tracker.set_total_items(self.statuses.len());
        self.find_new_poll_task(game_io);
    }

    fn find_new_poll_task(&mut self, game_io: &mut GameIO<Globals>) {
        if self.active_poll_task.is_some() {
            // already working on something
            return;
        }

        let next_address = &self
            .statuses
            .iter()
            .find(|(_, status)| *status == ServerStatus::Pending)
            .map(|(address, _)| address.clone());

        if let Some(address) = next_address.clone() {
            let subscription = game_io
                .globals_mut()
                .network
                .subscribe_to_server(address.clone());

            let task = game_io.spawn_local_task(async {
                let (send, receiver) = match subscription.await {
                    Some((sender, receiver)) => (sender, receiver),
                    None => return ServerStatus::InvalidAddress,
                };

                while !receiver.is_disconnected() {
                    send(Reliability::Unreliable, ClientPacket::VersionRequest);

                    async_sleep(SERVER_TICK_RATE).await;

                    let response = match receiver.try_recv() {
                        Ok(response) => response,
                        _ => continue,
                    };

                    let (version_id, version_iteration) = match response {
                        ServerPacket::VersionInfo {
                            version_id,
                            version_iteration,
                            ..
                        } => (version_id, version_iteration),
                        _ => continue,
                    };

                    if version_id != packets::VERSION_ID {
                        return ServerStatus::Incompatible;
                    }

                    if version_iteration > packets::VERSION_ITERATION {
                        return ServerStatus::TooNew;
                    } else if version_iteration < packets::VERSION_ITERATION {
                        return ServerStatus::TooOld;
                    }

                    return ServerStatus::Online;
                }

                ServerStatus::Offline
            });

            self.active_poll_task = Some((address, task));
        }
    }

    fn update_poll_task(&mut self, game_io: &mut GameIO<Globals>) {
        let completed_task = self
            .active_poll_task
            .as_ref()
            .map(|(_, task)| task.is_finished())
            .unwrap_or_default();

        if !completed_task {
            return;
        }

        // update stored status
        let (address, task) = self.active_poll_task.take().unwrap();
        let status = task.join().unwrap();

        use packets::address_parsing::strip_data;
        let address = strip_data(&address);

        for (stored_address, stored_status) in &mut self.statuses {
            if strip_data(stored_address) == address && *stored_status == ServerStatus::Pending {
                *stored_status = status;
            }
        }

        self.find_new_poll_task(game_io);
    }

    fn update_context_menu_options(&mut self, game_io: &GameIO<Globals>) {
        let global_save = &game_io.globals().global_save;

        if global_save.server_list.is_empty() {
            self.context_menu
                .set_options(game_io, &[("New", MenuOption::New)]);
        } else {
            self.context_menu.set_options(
                game_io,
                &[
                    ("New", MenuOption::New),
                    ("Edit", MenuOption::Edit),
                    ("Move", MenuOption::Move),
                    ("Delete", MenuOption::Delete),
                ],
            );
        }
    }

    fn update_context_menu(&mut self, game_io: &mut GameIO<Globals>) {
        let option = match self.context_menu.update(game_io, &self.ui_input_tracker) {
            Some(option) => option,
            None => return,
        };

        match option {
            MenuOption::Edit => {
                let index = self.scroll_tracker.selected_index();
                let scene = ServerEditScene::new(game_io, ServerEditProp::Edit(index));
                let transition =
                    ColorFadeTransition::new(game_io, Color::BLACK, DEFAULT_FADE_DURATION);

                self.next_scene = NextScene::new_push(scene).with_transition(transition);
            }
            MenuOption::New => {
                let index = self.scroll_tracker.selected_index();
                let scene = ServerEditScene::new(game_io, ServerEditProp::InsertAfter(index));
                let transition =
                    ColorFadeTransition::new(game_io, Color::BLACK, DEFAULT_FADE_DURATION);

                self.next_scene = NextScene::new_push(scene).with_transition(transition);
            }
            MenuOption::Move => {
                self.scroll_tracker.remember_index();
                self.context_menu.close();
            }
            MenuOption::Delete => {
                self.context_menu.close();

                let selected_index = self.scroll_tracker.selected_index();
                let global_save = &game_io.globals().global_save;
                let server_name = &global_save.server_list[selected_index].name;

                let event_sender = self.event_sender.clone();
                let interface =
                    TextboxQuestion::new(format!("Delete {server_name}?"), move |yes| {
                        if yes {
                            event_sender.send(Event::Delete).unwrap();
                        }
                    });

                self.textbox.push_interface(interface);
                self.textbox.open();
            }
        }
    }

    fn handle_events(&mut self, game_io: &mut GameIO<Globals>) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::Delete => {
                    let global_save = &mut game_io.globals_mut().global_save;
                    let selected_index = self.scroll_tracker.selected_index();

                    global_save.server_list.remove(selected_index);
                    self.statuses.remove(selected_index);
                    global_save.save();

                    self.scroll_tracker.set_total_items(self.statuses.len());

                    self.update_context_menu_options(game_io);
                }
            }
        }
    }
}

impl Scene<Globals> for ServerListScene {
    fn enter(&mut self, game_io: &mut GameIO<Globals>) {
        self.update_server_list(game_io);

        self.update_context_menu_options(game_io);
        self.context_menu.close();
    }

    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        self.update_poll_task(game_io);
        self.handle_events(game_io);

        self.time += 1;

        if game_io.is_in_transition() {
            return;
        }

        self.textbox.update(game_io);

        if self.textbox.is_open() {
            if self.textbox.is_complete() {
                self.textbox.close()
            }

            return;
        }

        self.ui_input_tracker.update(game_io);

        self.scroll_tracker.set_idle(self.context_menu.is_open());

        if self.context_menu.is_open() {
            self.update_context_menu(game_io);
            return;
        }

        let old_index = self.scroll_tracker.selected_index();

        self.scroll_tracker
            .handle_vertical_input(&self.ui_input_tracker);

        if self.scroll_tracker.selected_index() != old_index {
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.cursor_move_sfx);
        }

        if self.ui_input_tracker.is_active(Input::Option) {
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.cursor_select_sfx);

            // forget index to prevent issues from new servers shifting items around
            self.scroll_tracker.forget_index();
            self.context_menu.open();
        }

        if !self.statuses.is_empty() && self.ui_input_tracker.is_active(Input::Confirm) {
            let globals = game_io.globals_mut();
            globals.audio.play_sound(&globals.cursor_select_sfx);

            if let Some(index) = self.scroll_tracker.forget_index() {
                let selected_index = self.scroll_tracker.selected_index();

                self.statuses.swap(index, selected_index);
                globals.global_save.server_list.swap(index, selected_index);
                globals.global_save.save();
            } else {
                let server_list = &game_io.globals().global_save.server_list;
                let server_info = &server_list[self.scroll_tracker.selected_index()];
                let server_name = &server_info.name;

                let (_, status) = &self.statuses[self.scroll_tracker.selected_index()];

                if matches!(status, ServerStatus::Pending | ServerStatus::Online) {
                    // try connecting to the server
                    let scene = InitialConnectScene::new(game_io, server_info.address.clone());
                    let transition =
                        ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION);

                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }

                // see if there's a message to display
                if let Some(message) = status.create_message(server_name) {
                    let textbox_interface = TextboxMessage::new(message);
                    self.textbox.push_interface(textbox_interface);
                    self.textbox.open();
                }
            }
        } else if self.ui_input_tracker.is_active(Input::Cancel) {
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.cursor_cancel_sfx);

            if self.scroll_tracker.remembered_index().is_some() {
                self.scroll_tracker.forget_index();
            } else {
                let transition = PushTransition::new(
                    game_io,
                    game_io.globals().default_sampler.clone(),
                    Direction::Left,
                    DEFAULT_PUSH_DURATION,
                );

                self.next_scene = NextScene::new_pop().with_transition(transition);
            }
        }
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        let globals = game_io.globals();
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // render static pieces
        sprite_queue.draw_sprite(&self.bg_sprite);
        SceneTitle::new("SERVERS").draw(game_io, &mut sprite_queue);
        sprite_queue.draw_sprite(&self.list_box_sprite);

        // render rows
        let mut text_style = TextStyle::new_monospace(game_io, FontStyle::Thin)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR);

        const HEIGHT: f32 = 16.0;
        const INDICATOR_LEFT_MARGIN: f32 = 210.0;
        const TEXT_LEFT_MARGIN: f32 = 10.0;

        let mut y = self.list_start.y;
        let mut status_sprite = Sprite::new(
            self.bg_sprite.texture().clone(),
            globals.default_sampler.clone(),
        );

        for i in self.scroll_tracker.view_range() {
            let server_list = &game_io.globals().global_save.server_list;

            let name = &server_list[i].name;
            let (_address, status) = &self.statuses[i];

            (text_style.bounds).set_position(Vec2::new(self.list_start.x + TEXT_LEFT_MARGIN, y));
            text_style.draw(game_io, &mut sprite_queue, name);

            // draw status indicator
            self.animator.set_state(status.state());
            self.animator.set_loop_mode(AnimatorLoopMode::Loop);
            self.animator.sync_time(self.time - i as FrameTime * 3);
            self.animator.apply(&mut status_sprite);
            status_sprite.set_position(Vec2::new(INDICATOR_LEFT_MARGIN, y));
            sprite_queue.draw_sprite(&status_sprite);

            y += HEIGHT;
        }

        self.scroll_tracker.draw_scrollbar(&mut sprite_queue);

        if !self.statuses.is_empty() {
            self.scroll_tracker.draw_cursor(&mut sprite_queue);
        }

        // draw context menu
        self.context_menu.draw(game_io, &mut sprite_queue);

        // draw help
        const NEW_LABEL: &str = "OPTION:";
        const NEW_POSITION: Vec2 = Vec2::new(160.0, RESOLUTION_F.y - 15.0);

        text_style.monospace = false;
        text_style.font_style = FontStyle::Context;

        text_style.bounds.set_position(NEW_POSITION);
        text_style.draw(game_io, &mut sprite_queue, NEW_LABEL);

        text_style.color = Color::GREEN;
        text_style.bounds.set_position(NEW_POSITION);
        text_style.bounds.x += text_style.measure(NEW_LABEL).size.x + text_style.letter_spacing;
        text_style.draw(game_io, &mut sprite_queue, "MENU");

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
