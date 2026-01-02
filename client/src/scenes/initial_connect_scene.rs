use super::OverworldOnlineScene;
use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use packets::{ClientPacket, Reliability, ServerPacket};

enum Event {
    Subscribed(ClientPacketSender, ServerPacketReceiver),
    Failed { reason: Option<String> },
    Pop,
}

pub struct InitialConnectScene {
    camera: Camera,
    bg_sprite: Sprite,
    bg_animator: Animator,
    address: String,
    data: Option<String>,
    online_scene: Option<OverworldOnlineScene>,
    task: AsyncTask<()>,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    deferred_packets: Vec<ServerPacket>,
    textbox: Textbox,
    connection_started: bool,
    success: bool,
    next_scene: NextScene,
}

impl InitialConnectScene {
    pub fn new(game_io: &GameIO, address: String, data: Option<String>, animate: bool) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let (event_sender, event_receiver) = flume::unbounded();

        let subscription = globals.network.subscribe_to_server(address.clone());
        let task = game_io.spawn_local_task({
            let event_sender = event_sender.clone();
            async move {
                let event = match subscription.await {
                    Some((send, receiver)) => Event::Subscribed(send, receiver),
                    None => Event::Failed { reason: None },
                };

                event_sender.send(event).unwrap();
            }
        });

        let mut bg_sprite;
        let mut bg_animator = Animator::new();

        if animate {
            bg_sprite = assets.new_sprite(game_io, ResourcePaths::INITIAL_CONNECT_BG);
            bg_animator.load(assets, ResourcePaths::INITIAL_CONNECT_BG_ANIMATION);
            bg_animator.set_state("DEFAULT");
        } else {
            bg_sprite = assets.new_sprite(game_io, ResourcePaths::PIXEL);
            bg_sprite.set_size(RESOLUTION_F);
        }

        Self {
            camera: Camera::new_ui(game_io),
            bg_sprite,
            bg_animator,
            address,
            data,
            online_scene: None,
            task,
            event_sender,
            event_receiver,
            deferred_packets: Vec::new(),
            textbox: Textbox::new_navigation(game_io),
            connection_started: false,
            success: false,
            next_scene: NextScene::None,
        }
    }

    fn fail_with_message(&mut self, message: String) {
        let event_sender = self.event_sender.clone();
        let textbox_interface = TextboxMessage::new(message)
            .with_callback(move || event_sender.send(Event::Pop).unwrap());

        self.textbox.open();
        self.textbox.push_interface(textbox_interface);

        self.online_scene = None;
        self.success = false
    }

    fn handle_packets(&mut self, game_io: &mut GameIO) {
        let Some(online_scene) = &mut self.online_scene else {
            return;
        };

        // handle incoming packets
        while let Ok(packet) = online_scene.packet_receiver().try_recv() {
            match packet {
                ServerPacket::VersionInfo {
                    version_id,
                    version_iteration,
                } => {
                    let globals = Globals::from_resources(game_io);

                    if version_id != packets::VERSION_ID
                        || version_iteration != packets::VERSION_ITERATION
                    {
                        let message = globals.translate("server-generic-incompatible-message");
                        self.fail_with_message(message);
                        return;
                    }

                    if version_iteration < packets::VERSION_ITERATION {
                        let message = globals.translate("server-generic-ahead-message");
                        self.fail_with_message(message);
                        return;
                    }

                    if version_iteration > packets::VERSION_ITERATION {
                        let message = globals.translate("server-generic-behind-message");
                        self.fail_with_message(message);
                        return;
                    }

                    if !self.connection_started {
                        online_scene.start_connection(game_io, self.data.take());
                        self.connection_started = true;
                    }
                }
                ServerPacket::PackageList { packages } => {
                    online_scene.sync_assets(game_io, packages);

                    // nothing else to sync, request join
                    online_scene.request_join();
                }
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
                ServerPacket::LoadPackage { .. }
                | ServerPacket::InitiateEncounter { .. }
                | ServerPacket::InitiateNetplay { .. }
                | ServerPacket::Restrictions { .. }
                | ServerPacket::AddCard { .. }
                | ServerPacket::AddBlock { .. }
                | ServerPacket::EnablePlayableCharacter { .. } => {
                    self.deferred_packets.push(packet);
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
            self.success = false;
        }
    }
}

impl Scene for InitialConnectScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        let globals = Globals::from_resources_mut(game_io);
        globals.connected_to_server = true;
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.bg_animator.update();
        self.bg_animator.apply(&mut self.bg_sprite);

        self.handle_packets(game_io);

        self.textbox.update(game_io);

        if self.success && game_io.is_in_transition() {
            // leave events up to the next scene
            return;
        }

        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::Subscribed(send_packet, packet_receiver) => {
                    send_packet(Reliability::Reliable, ClientPacket::VersionRequest);

                    self.online_scene = Some(OverworldOnlineScene::new(
                        game_io,
                        self.address.clone(),
                        send_packet,
                        packet_receiver,
                    ));
                }
                Event::Failed { reason } => {
                    let globals = Globals::from_resources(game_io);
                    let message = match reason {
                        Some(reason) => globals
                            .translate_with_args("server-kicked", vec![("reason", reason.into())]),
                        None => globals.translate("server-connection-failed"),
                    };

                    self.fail_with_message(message);
                }
                Event::Pop => {
                    let transition = crate::transitions::new_connect(game_io);
                    self.next_scene = NextScene::new_pop().with_transition(transition);

                    self.textbox.close();
                }
            }
        }

        if self.bg_animator.is_complete() && self.success {
            let globals = Globals::from_resources_mut(game_io);
            globals.remove_namespace(PackageNamespace::Server);

            // reset restrictions before applying deferred packets
            globals.restrictions = Restrictions::default();

            let mut online_scene = self.online_scene.take().unwrap();

            for packet in std::mem::take(&mut self.deferred_packets) {
                online_scene.handle_packet(game_io, packet);
            }

            let globals = Globals::from_resources_mut(game_io);
            globals.assets.remove_unused_virtual_zips();

            // move to the network scene if we can and the animator completed
            let transition = crate::transitions::new_connect(game_io);
            self.next_scene = NextScene::new_swap(online_scene).with_transition(transition);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        sprite_queue.draw_sprite(&self.bg_sprite);

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
