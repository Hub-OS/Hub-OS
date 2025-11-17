use super::plugin_wrapper::PluginWrapper;
use super::{Net, PacketOrchestrator, ServerConfig};
use crate::jobs::{JobPromise, PromiseValue};
use crate::plugins::PluginInterface;
use crate::threads::{create_listening_thread, ListenerMessage, ThreadMessage};
use flume::{Receiver, Sender};
use packets::structures::ActorId;
use packets::{
    ClientAssetType, ClientPacket, Reliability, ServerCommPacket, ServerPacket, SERVER_TICK_RATE,
};
use std::cell::RefCell;
use std::collections::HashMap;
use std::net::{SocketAddr, UdpSocket};
use std::rc::Rc;
use std::time::Instant;

pub struct Server {
    player_id_map: HashMap<SocketAddr, ActorId>,
    plugin_wrapper: PluginWrapper,
    config: Rc<ServerConfig>,
    socket: Rc<UdpSocket>,
    net: Net,
    packet_orchestrator: Rc<RefCell<PacketOrchestrator>>,
    message_sender: Sender<ThreadMessage>,
    time: Instant,
    last_heartbeat: Instant,
    pending_server_polls: HashMap<SocketAddr, Vec<JobPromise>>,
}

impl Server {
    pub(super) fn new(
        config: ServerConfig,
        socket: UdpSocket,
        mut plugin_wrapper: PluginWrapper,
        message_sender: Sender<ThreadMessage>,
    ) -> Self {
        let config = Rc::new(config);
        let socket = Rc::new(socket);

        let packet_orchestrator = Rc::new(RefCell::new(PacketOrchestrator::new(
            socket.clone(),
            config.clone(),
        )));

        let mut net = Net::new(
            packet_orchestrator.clone(),
            config.clone(),
            message_sender.clone(),
        );
        plugin_wrapper.init(&mut net);

        Self {
            player_id_map: HashMap::new(),
            plugin_wrapper,
            config,
            socket,
            net,
            packet_orchestrator,
            message_sender,
            time: Instant::now(),
            last_heartbeat: Instant::now(),
            pending_server_polls: HashMap::new(),
        }
    }

    pub async fn start(
        &mut self,
        message_receiver: Receiver<ThreadMessage>,
    ) -> Result<(), Box<dyn std::error::Error>> {
        use futures::FutureExt;
        use futures::StreamExt;

        log::info!("Server started");

        let (listener_sender, listener_receiver) = flume::unbounded();
        create_listening_thread(
            self.message_sender.clone(),
            listener_receiver,
            self.socket.try_clone()?,
            (*self.config).clone(),
        );

        let sleep_future = async_std::task::sleep(SERVER_TICK_RATE).fuse();
        let mut message_stream = message_receiver.stream();

        futures::pin_mut!(sleep_future);

        loop {
            futures::select_biased! {
                _ = sleep_future => {
                    self.tick(listener_sender.clone()).await;
                    sleep_future.set(async_std::task::sleep(SERVER_TICK_RATE).fuse());
                }
                message = message_stream.select_next_some() => {
                    match message {
                        ThreadMessage::NewConnection { socket_address } => {
                            let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
                            packet_orchestrator.create_connection(socket_address);

                            let receivers = packet_orchestrator.take_packet_receivers();
                            listener_sender.send(ListenerMessage::NewConnections { receivers }).unwrap();
                        }
                        ThreadMessage::ServerCommPacket { socket_address, packet } => {
                            self.handle_server_comm_packet(
                                socket_address,
                                packet,
                            );
                        }
                        ThreadMessage::ClientPacket { socket_address, packet } => {
                            self.handle_client_packet(
                                socket_address,
                                packet,
                            );
                        }
                        ThreadMessage::NetplayPacket  { socket_address, packet } => {
                            let packet_orchestrator = self.packet_orchestrator.borrow_mut();
                            packet_orchestrator.forward_netplay_packet(socket_address, packet);
                        }
                        ThreadMessage::MessageServer { socket_address, data } => {
                            let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
                            packet_orchestrator.create_connection(socket_address);
                            packet_orchestrator.send_server_comm(
                                socket_address,
                                Reliability::ReliableOrdered,
                                ServerCommPacket::Message { data }
                            );
                        }
                        ThreadMessage::PollServer { socket_address, promise } => {
                            let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
                            packet_orchestrator.create_connection(socket_address);
                            packet_orchestrator.send_server_comm(
                                socket_address,
                                Reliability::Reliable,
                                ServerCommPacket::Poll
                            );

                            if let Some(pending) = self.pending_server_polls.get_mut(&socket_address) {
                                pending.push(promise);
                            } else {
                                self.pending_server_polls.insert(socket_address, vec![promise]);
                            }
                        }
                    }
                }
            };
        }
    }

    async fn tick(&mut self, listener_sender: Sender<ListenerMessage>) {
        let elapsed_time = self.time.elapsed();
        self.time = Instant::now();

        self.plugin_wrapper
            .tick(&mut self.net, elapsed_time.as_secs_f32());

        // kick silent clients
        let mut kick_list = self.packet_orchestrator.borrow_mut().tick();

        // remove packet listeners for timed out connections
        let dropped_addresses = kick_list.iter().map(|boot| boot.socket_address).collect();

        listener_sender
            .send(ListenerMessage::DropConnections {
                addresses: dropped_addresses,
            })
            .unwrap();

        // add clients kicked by plugins to the kick list
        kick_list.extend(self.net.take_kick_list());

        // actually kick clients
        for boot in kick_list {
            self.disconnect_client(boot.socket_address, &boot.reason, boot.warp_out);

            if boot.notify_client {
                // send reason
                self.packet_orchestrator.borrow_mut().send(
                    boot.socket_address,
                    Reliability::ReliableOrdered,
                    ServerPacket::Kick {
                        reason: boot.reason,
                    },
                );
            }
        }

        self.net.tick();

        if self.last_heartbeat.elapsed().as_secs_f32() >= self.config.heartbeat_rate {
            self.packet_orchestrator
                .borrow_mut()
                .broadcast_to_clients(Reliability::Reliable, ServerPacket::Heartbeat);

            self.last_heartbeat = self.time;
        }

        // handle connections created in tick
        let receivers = self
            .packet_orchestrator
            .borrow_mut()
            .take_packet_receivers();

        listener_sender
            .send(ListenerMessage::NewConnections { receivers })
            .unwrap();
    }

    fn handle_server_comm_packet(&mut self, socket_address: SocketAddr, packet: ServerCommPacket) {
        if self.config.args.log_packets {
            let packet_name: &'static str = (&packet).into();
            log::debug!("Received ServerCommPacket::{packet_name} packet from {socket_address}");
        }

        match packet {
            ServerCommPacket::Poll => {
                let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
                packet_orchestrator.send_server_comm(
                    socket_address,
                    Reliability::Reliable,
                    ServerCommPacket::Alive,
                );
            }
            ServerCommPacket::Alive => {
                if let Some(promises) = self.pending_server_polls.remove(&socket_address) {
                    for mut promise in promises {
                        promise.set_value(PromiseValue::ServerPolled {});
                    }
                }
            }
            ServerCommPacket::Message { data } => {
                self.plugin_wrapper
                    .handle_server_message(&mut self.net, socket_address, &data);
            }
        }
    }

    fn handle_client_packet(&mut self, socket_address: SocketAddr, client_packet: ClientPacket) {
        let net = &mut self.net;

        if self.config.args.log_packets {
            let packet_name: &'static str = (&client_packet).into();
            log::debug!("Received {packet_name} packet from {socket_address}");
        }

        if let Some(&player_id) = self.player_id_map.get(&socket_address) {
            match client_packet {
                ClientPacket::VersionRequest => {
                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::Reliable,
                        ServerPacket::new_version_info(),
                    );
                }
                ClientPacket::AssetFound { path, hash } => {
                    let mut is_valid = false;

                    if let Some(asset) = net.get_asset(&path) {
                        is_valid = asset.hash == hash;
                    }

                    if let Some(client) = net.get_client_mut(player_id) {
                        if is_valid {
                            client.cached_assets.insert(path);
                        } else if !client.cached_assets.contains(&path) {
                            // tell the client to delete this asset if we haven't already sent new data for it
                            self.packet_orchestrator.borrow_mut().send(
                                socket_address,
                                Reliability::ReliableOrdered,
                                ServerPacket::RemoveAsset { path },
                            );
                        }
                    }
                }
                ClientPacket::Asset { asset_type, data } => {
                    if data.len() > self.config.args.player_asset_limit {
                        let reason = format!(
                            "Avatar asset larger than {}KiB",
                            self.config.args.player_asset_limit / 1024
                        );

                        net.kick_player(player_id, &reason, true);
                    }

                    if let Some(client) = net.get_client_mut(player_id) {
                        match asset_type {
                            ClientAssetType::Texture => {
                                client.texture_buffer = data;
                            }
                            ClientAssetType::Animation => {
                                client.animation_buffer = data;
                            }
                            ClientAssetType::MugshotTexture => {
                                client.mugshot_texture_buffer = data;
                            }
                            ClientAssetType::MugshotAnimation => {
                                client.mugshot_animation_buffer = data;
                            }
                        };
                    }
                }
                ClientPacket::Authorize { .. } | ClientPacket::Login { .. } => {
                    if self.config.args.log_packets {
                        log::debug!(
                            "Previous packet shouldn't be sent if the client is already connected"
                        );
                    }
                }
                ClientPacket::RequestJoin => {
                    net.spawn_client(player_id);

                    self.plugin_wrapper.handle_player_connect(net, player_id);

                    net.connect_client(player_id);

                    if self.config.args.log_connections {
                        log::debug!("{player_id:?} connected");
                    }
                }
                ClientPacket::Logout => {
                    self.disconnect_client(socket_address, "Leaving", true);
                }
                ClientPacket::Position {
                    creation_time,
                    x,
                    y,
                    z,
                    direction,
                } => {
                    if let Some(client) = net.get_client_mut(player_id) {
                        if client.ready && creation_time > client.area_join_time {
                            if !client.actor.position_matches(x, y, z) {
                                self.plugin_wrapper
                                    .handle_player_move(net, player_id, x, y, z);
                            }

                            net.update_player_position(player_id, x, y, z, direction);
                        }
                    }
                }
                ClientPacket::Ready { time } => {
                    if let Some(client) = net.get_client_mut(player_id) {
                        client.area_join_time = time;
                        client.actor.x = client.warp_x;
                        client.actor.y = client.warp_y;
                        client.actor.z = client.warp_z;

                        if client.transferring {
                            self.plugin_wrapper.handle_player_transfer(net, player_id);
                        } else {
                            self.plugin_wrapper.handle_player_join(net, player_id);
                        }

                        net.mark_client_ready(player_id);
                    }
                }
                ClientPacket::TransferredOut => {
                    net.complete_transfer(player_id);
                }
                ClientPacket::CustomWarp { tile_object_id } => {
                    self.plugin_wrapper
                        .handle_custom_warp(net, player_id, tile_object_id);
                }
                ClientPacket::Boost {
                    health_boost,
                    augments,
                } => {
                    if let Some(client) = net.get_client_mut(player_id) {
                        let player_data = &mut client.player_data;

                        if player_data.health == player_data.max_health() {
                            player_data.health = player_data.base_health + health_boost;
                        }

                        player_data.health_boost = health_boost;
                        player_data.health = player_data.max_health().min(player_data.health);

                        // max_health is applied in the client as well
                        // but reapplied through a packet in case of an in-flight packet overriding it
                        self.packet_orchestrator.borrow_mut().send(
                            socket_address,
                            Reliability::ReliableOrdered,
                            ServerPacket::Health {
                                health: player_data.health,
                            },
                        );

                        self.plugin_wrapper
                            .handle_player_augment(net, player_id, &augments);
                    }
                }
                ClientPacket::AvatarChange {
                    name,
                    element,
                    base_health,
                } => {
                    if let Some((texture_path, animation_path)) = net.store_player_assets(player_id)
                    {
                        net.update_player_data(player_id, name, element, base_health);

                        if let Some(player_data) = net.get_player_data(player_id) {
                            // health is applied in the client as well
                            // but reapplied through a packet in case of an in-flight packet overriding it
                            self.packet_orchestrator.borrow_mut().send(
                                socket_address,
                                Reliability::ReliableOrdered,
                                ServerPacket::Health {
                                    health: player_data.health,
                                },
                            );
                        }

                        let prevent_default = self.plugin_wrapper.handle_player_avatar_change(
                            net,
                            player_id,
                            &texture_path,
                            &animation_path,
                        );

                        if !prevent_default {
                            net.set_player_avatar(player_id, &texture_path, &animation_path);
                        }
                    }
                }
                ClientPacket::Emote { emote_id } => {
                    let prevent_default = self
                        .plugin_wrapper
                        .handle_player_emote(net, player_id, &emote_id);

                    if !prevent_default {
                        net.set_player_emote(player_id, emote_id);
                    }
                }
                ClientPacket::AnimationUpdated {
                    animation,
                    loop_animation,
                    x,
                    y,
                    z,
                } => {
                    if let Some(client) = net.get_client_mut(player_id) {
                        if client.actor.position_matches(x, y, z) {
                            client.actor.current_animation = Some(animation);
                            client.actor.loop_animation = loop_animation;
                        }
                    }
                }
                ClientPacket::ObjectInteraction {
                    tile_object_id,
                    button,
                } => {
                    if !net.is_player_busy(player_id) {
                        self.plugin_wrapper.handle_object_interaction(
                            net,
                            player_id,
                            tile_object_id,
                            button,
                        );
                    }
                }
                ClientPacket::ActorInteraction { actor_id, button } => {
                    if !net.is_player_busy(player_id) {
                        self.plugin_wrapper
                            .handle_actor_interaction(net, player_id, actor_id, button);
                    }
                }
                ClientPacket::TileInteraction { x, y, z, button } => {
                    if !net.is_player_busy(player_id) {
                        self.plugin_wrapper
                            .handle_tile_interaction(net, player_id, x, y, z, button);
                    }
                }
                ClientPacket::TextBoxResponse { response } => {
                    self.plugin_wrapper
                        .handle_textbox_response(net, player_id, response);

                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::ReliableOrdered,
                        ServerPacket::TextBoxResponseAck,
                    );
                }
                ClientPacket::PromptResponse { response } => {
                    self.plugin_wrapper
                        .handle_prompt_response(net, player_id, response);

                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::ReliableOrdered,
                        ServerPacket::TextBoxResponseAck,
                    );
                }
                ClientPacket::BoardOpen => {
                    self.plugin_wrapper.handle_board_open(net, player_id);
                }
                ClientPacket::BoardClose => {
                    self.plugin_wrapper.handle_board_close(net, player_id);
                }
                ClientPacket::PostRequest => {
                    self.plugin_wrapper.handle_post_request(net, player_id);
                }
                ClientPacket::PostSelection { post_id } => {
                    self.plugin_wrapper
                        .handle_post_selection(net, player_id, &post_id);

                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::ReliableOrdered,
                        ServerPacket::SelectionAck,
                    );
                }
                ClientPacket::ShopOpen => {
                    self.plugin_wrapper.handle_shop_open(net, player_id);
                }
                ClientPacket::ShopLeave => {
                    self.plugin_wrapper.handle_shop_leave(net, player_id);

                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::ReliableOrdered,
                        ServerPacket::SelectionAck,
                    );
                }
                ClientPacket::ShopClose => {
                    self.plugin_wrapper.handle_shop_close(net, player_id);
                }
                ClientPacket::ShopPurchase { item_id } => {
                    self.plugin_wrapper
                        .handle_shop_purchase(net, player_id, &item_id);

                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::ReliableOrdered,
                        ServerPacket::SelectionAck,
                    );
                }
                ClientPacket::ShopDescriptionRequest { item_id } => {
                    self.plugin_wrapper
                        .handle_shop_description_request(net, player_id, &item_id);

                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::ReliableOrdered,
                        ServerPacket::SelectionAck,
                    );
                }
                ClientPacket::ItemUse { item_id } => {
                    self.plugin_wrapper
                        .handle_item_use(net, player_id, &item_id);

                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::ReliableOrdered,
                        ServerPacket::SelectionAck,
                    );
                }
                ClientPacket::NetplayPreferences { force_relay } => {
                    if let Some(client) = net.get_client_mut(player_id) {
                        client.force_relay = force_relay;
                    }
                }
                ClientPacket::EncounterStart => {
                    let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();

                    if packet_orchestrator.disconnect_from_netplay(socket_address) {
                        log::warn!("Received EncounterStart while already in a battle?");
                    }

                    if let Some(client) = net.get_client(player_id) {
                        if let Some(info) = client.battle_tracker.front() {
                            packet_orchestrator.configure_netplay_destinations(
                                socket_address,
                                info.player_index,
                                info.remote_addresses.clone(),
                            );
                        }
                    }
                }
                ClientPacket::BattleMessage { message } => {
                    self.plugin_wrapper.handle_battle_message(
                        net,
                        player_id,
                        // the wrapper will resolve the battle_id
                        Default::default(),
                        &message,
                    );
                }
                ClientPacket::BattleResults { battle_stats } => {
                    self.packet_orchestrator
                        .borrow_mut()
                        .disconnect_from_netplay(socket_address);

                    self.plugin_wrapper
                        .handle_battle_results(net, player_id, &battle_stats);
                }
            }
        } else {
            match client_packet {
                ClientPacket::VersionRequest => {
                    self.packet_orchestrator.borrow_mut().send(
                        socket_address,
                        Reliability::Reliable,
                        ServerPacket::new_version_info(),
                    );
                }
                ClientPacket::Authorize {
                    origin_address,
                    identity,
                    data,
                } => {
                    self.plugin_wrapper.handle_authorization(
                        net,
                        &identity,
                        &origin_address,
                        &data,
                    );
                }
                ClientPacket::Login {
                    username,
                    identity,
                    data,
                } => {
                    let player_id = net.add_client(socket_address, username, identity);

                    self.player_id_map.insert(socket_address, player_id);

                    self.plugin_wrapper
                        .handle_player_request(net, player_id, &data);

                    net.share_packages(player_id);
                }
                _ => {
                    if self.config.args.log_packets {
                        log::debug!(
                            "Previous packet requires connection yet the client is disconnected ",
                        );
                    }
                }
            }
        }
    }

    fn disconnect_client(&mut self, socket_address: SocketAddr, reason: &str, warp_out: bool) {
        if let Some(player_id) = self.player_id_map.remove(&socket_address) {
            self.plugin_wrapper
                .handle_player_disconnect(&mut self.net, player_id);

            self.net.remove_player(player_id, warp_out);

            if self.config.args.log_connections {
                log::debug!("{player_id:?} disconnected for {reason}");
            }
        } else if self.config.args.log_connections {
            log::debug!("{socket_address} disconnected for {reason}");
        }
    }
}
