use super::{InitialConnectScene, ServerEditProp, ServerEditScene};
use crate::bindable::SpriteColorMode;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;

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
    background: Background,
    frame: SubSceneFrame,
    scrollable_frame: ScrollableFrame,
    status_animator: Animator,
    status_sprite: Sprite,
    statuses: Vec<(String, Option<ServerStatus>)>, // address, statuses
    scroll_tracker: ScrollTracker,
    ui_input_tracker: UiInputTracker,
    context_menu: ContextMenu<MenuOption>,
    option_tip: OptionTip,
    active_poll_task: Option<(String, AsyncTask<ServerStatus>)>,
    textbox: Textbox,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    music_stack_len: usize,
    time: FrameTime,
    next_scene: NextScene,
}

impl ServerListScene {
    pub fn new(game_io: &GameIO) -> Box<Self> {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut layout_animator =
            Animator::load_new(assets, ResourcePaths::SERVER_LIST_LAYOUT_ANIMATION);
        layout_animator.set_state("DEFAULT");

        // list box
        let frame_bounds = Rect::from_corners(
            layout_animator.point("LIST_START").unwrap_or_default(),
            layout_animator.point("LIST_END").unwrap_or_default(),
        );

        let scrollable_frame = ScrollableFrame::new(game_io, frame_bounds);

        // scroll tracker
        let cursor_start = scrollable_frame.body_bounds().top_left() + Vec2::new(0.0, 5.0);

        let mut scroll_tracker = ScrollTracker::new(game_io, 7);
        scroll_tracker.define_cursor(cursor_start, 16.0);
        scroll_tracker.define_scrollbar(
            scrollable_frame.scroll_start(),
            scrollable_frame.scroll_end(),
        );

        // context menu
        let context_position = layout_animator.point("CONTEXT_MENU").unwrap_or_default();
        let context_menu = ContextMenu::new(game_io, "OPTIONS", context_position);

        // option tip
        let option_tip_top_right = layout_animator
            .point("MENU_TIP_TOP_RIGHT")
            .unwrap_or_default();
        let option_tip = OptionTip::new(String::from("MENU"), option_tip_top_right);

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        Box::new(Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_top_bar(true),
            scrollable_frame,
            status_animator: Animator::load_new(
                assets,
                ResourcePaths::SERVER_LIST_STATUS_ANIMATION,
            ),
            status_sprite: assets.new_sprite(game_io, ResourcePaths::SERVER_LIST_STATUS),
            statuses: Vec::new(),
            scroll_tracker,
            ui_input_tracker: UiInputTracker::new(),
            context_menu,
            option_tip,
            active_poll_task: None,
            textbox: Textbox::new_navigation(game_io),
            event_sender,
            event_receiver,
            music_stack_len: globals.audio.music_stack_len(),
            time: 0,
            next_scene: NextScene::None,
        })
    }

    fn status_state(status: Option<ServerStatus>) -> &'static str {
        let Some(status) = status else {
            return "PENDING";
        };

        match status {
            ServerStatus::Online => "ONLINE",
            ServerStatus::Offline => "OFFLINE",
            ServerStatus::TooOld
            | ServerStatus::TooNew
            | ServerStatus::Incompatible
            | ServerStatus::InvalidAddress => "ERROR",
        }
    }

    fn create_message(status: Option<ServerStatus>, server_name: &str) -> Option<String> {
        let Some(status) = status else {
            return None;
        };

        let message = match status {
            ServerStatus::Online => {
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

    fn update_server_list(&mut self, game_io: &mut GameIO) {
        use packets::address_parsing::strip_data;

        let globals = game_io.resource::<Globals>().unwrap();
        let server_list = &globals.global_save.server_list;

        for (i, server_info) in server_list.iter().enumerate() {
            let new_address = &server_info.address;

            let address = match self.statuses.get(i) {
                Some((address, _)) => address,
                None => {
                    // status list is shorter, just need to append
                    self.statuses.push((new_address.clone(), None));
                    continue;
                }
            };

            if strip_data(address) == strip_data(new_address) {
                continue;
            }

            self.statuses[i] = (new_address.clone(), None);
        }

        self.scroll_tracker.set_total_items(self.statuses.len());
        self.find_new_poll_task(game_io);
    }

    fn find_new_poll_task(&mut self, game_io: &mut GameIO) {
        if self.active_poll_task.is_some() {
            // already working on something
            return;
        }

        let next_address = &self
            .statuses
            .iter()
            .find(|(_, status)| status.is_none())
            .map(|(address, _)| address.clone());

        if let Some(address) = next_address.clone() {
            let globals = game_io.resource_mut::<Globals>().unwrap();
            let subscription = globals.network.subscribe_to_server(address.clone());

            let task = game_io.spawn_local_task(async {
                let (send, receiver) = match subscription.await {
                    Some((sender, receiver)) => (sender, receiver),
                    None => return ServerStatus::InvalidAddress,
                };

                Network::poll_server(&send, &receiver).await
            });

            self.active_poll_task = Some((address, task));
        }
    }

    fn update_poll_task(&mut self, game_io: &mut GameIO) {
        let completed_task =
            matches!(&self.active_poll_task, Some((_, task)) if task.is_finished());

        if !completed_task {
            return;
        }

        // update stored status
        let (address, task) = self.active_poll_task.take().unwrap();
        let status = task.join().unwrap();

        use packets::address_parsing::strip_data;
        let address = strip_data(&address);

        for (stored_address, stored_status) in &mut self.statuses {
            if strip_data(stored_address) == address && stored_status.is_none() {
                *stored_status = Some(status);
            }
        }

        self.find_new_poll_task(game_io);
    }

    fn update_context_menu_options(&mut self, game_io: &GameIO) {
        let global_save = &game_io.resource::<Globals>().unwrap().global_save;

        if global_save.server_list.is_empty() {
            self.context_menu
                .set_options(game_io, &[("NEW", MenuOption::New)]);
        } else {
            self.context_menu.set_options(
                game_io,
                &[
                    ("NEW", MenuOption::New),
                    ("EDIT", MenuOption::Edit),
                    ("MOVE", MenuOption::Move),
                    ("DELETE", MenuOption::Delete),
                ],
            );
        }
    }

    fn update_context_menu(&mut self, game_io: &mut GameIO) {
        let option = match self.context_menu.update(game_io, &self.ui_input_tracker) {
            Some(option) => option,
            None => return,
        };

        match option {
            MenuOption::Edit => {
                let index = self.scroll_tracker.selected_index();
                let scene = ServerEditScene::new(game_io, ServerEditProp::Edit(index));
                let transition = crate::transitions::new_sub_scene(game_io);

                self.next_scene = NextScene::new_push(scene).with_transition(transition);
            }
            MenuOption::New => {
                let index = self.scroll_tracker.selected_index();
                let scene = ServerEditScene::new(game_io, ServerEditProp::InsertAfter(index));
                let transition = crate::transitions::new_sub_scene(game_io);

                self.next_scene = NextScene::new_push(scene).with_transition(transition);
            }
            MenuOption::Move => {
                self.scroll_tracker.remember_index();
                self.context_menu.close();
            }
            MenuOption::Delete => {
                self.context_menu.close();

                let selected_index = self.scroll_tracker.selected_index();
                let global_save = &game_io.resource::<Globals>().unwrap().global_save;
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

    fn handle_events(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::Delete => {
                    let global_save = &mut game_io.resource_mut::<Globals>().unwrap().global_save;
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

impl Scene for ServerListScene {
    fn enter(&mut self, game_io: &mut GameIO) {
        self.update_server_list(game_io);

        self.update_context_menu_options(game_io);
        self.context_menu.close();

        // clean up music after we've left every server
        let globals = game_io.resource_mut::<Globals>().unwrap();

        globals.audio.truncate_music_stack(self.music_stack_len);

        if !globals.audio.is_music_playing() {
            globals.audio.restart_music();
        }

        // reset restrictions
        globals.restrictions = Restrictions::default();
    }

    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
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
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        if self.ui_input_tracker.is_active(Input::Option) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);

            // forget index to prevent issues from new servers shifting items around
            self.scroll_tracker.forget_index();
            self.context_menu.open();
        }

        if !self.statuses.is_empty() && self.ui_input_tracker.is_active(Input::Confirm) {
            let globals = game_io.resource_mut::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);

            if let Some(index) = self.scroll_tracker.forget_index() {
                let selected_index = self.scroll_tracker.selected_index();

                self.statuses.swap(index, selected_index);
                globals.global_save.server_list.swap(index, selected_index);
                globals.global_save.save();
            } else {
                let globals = game_io.resource::<Globals>().unwrap();
                let server_list = &globals.global_save.server_list;
                let server_info = &server_list[self.scroll_tracker.selected_index()];
                let server_name = &server_info.name;

                let (_, status) = &self.statuses[self.scroll_tracker.selected_index()];

                if matches!(status, None | Some(ServerStatus::Online)) {
                    // play join sfx
                    globals.audio.push_music_stack();
                    globals.audio.play_sound(&globals.sfx.transmission);

                    // try connecting to the server
                    let scene =
                        InitialConnectScene::new(game_io, server_info.address.clone(), None, true);
                    let transition = crate::transitions::new_connect(game_io);

                    self.next_scene = NextScene::new_push(scene).with_transition(transition);
                }

                // see if there's a message to display
                if let Some(message) = Self::create_message(*status, server_name) {
                    let textbox_interface = TextboxMessage::new(message);
                    self.textbox.push_interface(textbox_interface);
                    self.textbox.open();
                }
            }
        } else if self.ui_input_tracker.is_active(Input::Cancel) {
            let globals = game_io.resource::<Globals>().unwrap();

            if self.scroll_tracker.remembered_index().is_some() {
                globals.audio.play_sound(&globals.sfx.cursor_cancel);

                self.scroll_tracker.forget_index();
            } else {
                globals.audio.play_sound(&globals.sfx.cursor_cancel);

                let transition = crate::transitions::new_scene_pop(game_io);
                self.next_scene = NextScene::new_pop().with_transition(transition);
            }
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // render static pieces
        self.background.draw(game_io, render_pass);

        self.frame.draw(&mut sprite_queue);
        SceneTitle::new("SERVER LIST").draw(game_io, &mut sprite_queue);

        self.scrollable_frame.draw(game_io, &mut sprite_queue);

        // render rows
        let mut text_style = TextStyle::new_monospace(game_io, FontStyle::Thin)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR);

        const HEIGHT: f32 = 16.0;
        const INDICATOR_LEFT_MARGIN: f32 = 210.0;
        const TEXT_OFFSET: Vec2 = Vec2::new(10.0, 3.0);

        let list_start = self.scrollable_frame.body_bounds().top_left();
        let mut y = list_start.y;

        for i in self.scroll_tracker.view_range() {
            let globals = game_io.resource::<Globals>().unwrap();
            let server_list = &globals.global_save.server_list;

            let name = &server_list[i].name;
            let (_address, status) = &self.statuses[i];

            (text_style.bounds).set_position(Vec2::new(list_start.x, y) + TEXT_OFFSET);
            text_style.draw(game_io, &mut sprite_queue, name);

            // draw status indicator
            self.status_animator.set_state(Self::status_state(*status));
            self.status_animator.set_loop_mode(AnimatorLoopMode::Loop);
            self.status_animator
                .sync_time(self.time - i as FrameTime * 3);
            self.status_animator.apply(&mut self.status_sprite);
            self.status_sprite
                .set_position(Vec2::new(INDICATOR_LEFT_MARGIN, y));
            sprite_queue.draw_sprite(&self.status_sprite);

            y += HEIGHT;
        }

        self.scroll_tracker.draw_scrollbar(&mut sprite_queue);

        if !self.statuses.is_empty() {
            self.scroll_tracker.draw_cursor(&mut sprite_queue);
        }

        // draw context menu
        self.context_menu.draw(game_io, &mut sprite_queue);

        // draw help
        self.option_tip.draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
