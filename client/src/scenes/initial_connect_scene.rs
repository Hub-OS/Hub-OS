use super::OverworldOnlineScene;
use crate::bindable::SpriteColorMode;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::transitions::{ColorFadeTransition, DEFAULT_FADE_DURATION};
use framework::prelude::*;
use packets::{ClientPacket, Reliability, ServerPacket, SERVER_TICK_RATE};

enum Event {
    ReceivedPayloadSize(ClientPacketSender, ServerPacketReceiver, u16),
    Success,
    Failed { reason: Option<String> },
    Pop,
}

pub struct InitialConnectScene {
    camera: Camera,
    bg_sprite: Sprite,
    bg_animator: Animator,
    address: String,
    online_scene: Option<OverworldOnlineScene>,
    task: AsyncTask<()>,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    textbox: Textbox,
    success: bool,
    next_scene: NextScene<Globals>,
}

impl InitialConnectScene {
    pub fn new(game_io: &GameIO<Globals>, address: String) -> Self {
        let globals = game_io.globals();
        let assets = &globals.assets;

        let (event_sender, event_receiver) = flume::unbounded();

        let subscription = globals.network.subscribe_to_server(address.clone());
        let task = game_io.spawn_local_task({
            let event_sender = event_sender.clone();
            async move {
                let (send, receiver) = match subscription.await {
                    Some((send, receiver)) => (send, receiver),
                    None => {
                        event_sender.send(Event::Failed { reason: None }).unwrap();
                        return;
                    }
                };

                while !receiver.is_disconnected() {
                    send(Reliability::Unreliable, ClientPacket::VersionRequest);

                    async_sleep(SERVER_TICK_RATE).await;

                    let response = match receiver.try_recv() {
                        Ok(response) => response,
                        _ => continue,
                    };

                    let (version_id, version_iteration, max_payload_size) = match response {
                        ServerPacket::VersionInfo {
                            version_id,
                            version_iteration,
                            max_payload_size,
                        } => (version_id, version_iteration, max_payload_size),
                        ServerPacket::Kick { reason } => {
                            event_sender
                                .send(Event::Failed {
                                    reason: Some(reason),
                                })
                                .unwrap();
                            return;
                        }
                        _ => continue,
                    };

                    if version_id != packets::VERSION_ID
                        || version_iteration != packets::VERSION_ITERATION
                    {
                        event_sender.send(Event::Failed { reason: None }).unwrap();
                        return;
                    }

                    let event =
                        Event::ReceivedPayloadSize(send, receiver.clone(), max_payload_size);

                    event_sender.send(event).unwrap();
                    return;
                }

                // we return on success, so this is a failure
                event_sender.send(Event::Failed { reason: None }).unwrap();
            }
        });

        Self {
            camera: Camera::new_ui(game_io),
            bg_sprite: assets.new_sprite(game_io, ResourcePaths::INITIAL_CONNECT_BG),
            bg_animator: Animator::load_new(assets, ResourcePaths::INITIAL_CONNECT_BG_ANIMATION)
                .with_state("DEFAULT"),
            address,
            online_scene: None,
            task,
            event_sender,
            event_receiver,
            textbox: Textbox::new_navigation(game_io),
            success: false,
            next_scene: NextScene::None,
        }
    }
}

impl Scene<Globals> for InitialConnectScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        self.bg_animator.update();
        self.bg_animator.apply(&mut self.bg_sprite);

        if let Some(online_scene) = &mut self.online_scene {
            // handle incoming packets
            while let Ok(packet) = online_scene.packet_receiver().try_recv() {
                match packet {
                    ServerPacket::Kick { reason } => {
                        let event = Event::Failed {
                            reason: Some(reason),
                        };

                        self.event_sender.send(event).unwrap();
                    }
                    ServerPacket::CompleteConnection => {
                        self.success = true;
                        online_scene.handle_packet(game_io, packet);
                    }
                    packet => {
                        online_scene.handle_packet(game_io, packet);
                    }
                }
            }

            if online_scene.packet_receiver().is_disconnected() {
                self.online_scene.take();
                self.event_sender
                    .send(Event::Failed { reason: None })
                    .unwrap();
            }
        }

        self.textbox.update(game_io);

        if self.success && game_io.is_in_transition() {
            // leave events up to the next scene
            return;
        }

        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::ReceivedPayloadSize(send_packet, packet_receiver, max_payload_size) => {
                    let online_scene = OverworldOnlineScene::new(
                        game_io,
                        self.address.clone(),
                        max_payload_size,
                        send_packet,
                        packet_receiver,
                    );

                    online_scene.start_connection(game_io);

                    self.online_scene = Some(online_scene);
                }
                Event::Success => {
                    self.success = true;
                }
                Event::Failed { reason } => {
                    let message = match reason {
                        Some(reason) => format!("We've been kicked: {:?}", reason),
                        None => String::from("Connection failed."),
                    };

                    let event_sender = self.event_sender.clone();
                    let textbox_interface = TextboxMessage::new(message)
                        .with_callback(move || event_sender.send(Event::Pop).unwrap());

                    self.textbox.open();
                    self.textbox.push_interface(textbox_interface);
                }
                Event::Pop => {
                    let transition =
                        ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION);
                    self.next_scene = NextScene::new_pop().with_transition(transition);

                    self.textbox.close();
                }
            }
        }

        if self.bg_animator.is_complete() && self.success {
            // move to the network scene if we can and the animator completed
            let transition = ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION);

            self.next_scene =
                NextScene::new_swap(self.online_scene.take().unwrap()).with_transition(transition);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        sprite_queue.draw_sprite(&self.bg_sprite);

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
