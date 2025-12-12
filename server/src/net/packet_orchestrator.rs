use packets::structures::ActorId;
use packets::{
    ChannelSender, ConnectionBuilder, NetplayPacket, PacketChannels, PacketReceiver, PacketSender,
    Reliability, ServerCommPacket, ServerPacket, serialize,
};
use slotmap::SlotMap;
use std::collections::{HashMap, HashSet};
use std::net::{SocketAddr, UdpSocket};
use std::rc::Rc;
use std::sync::Arc;

use super::ServerConfig;
use super::boot::Boot;

struct Connection {
    pub socket_address: SocketAddr,
    pub client_id: Option<ActorId>,
    pub netplay_index: usize,
    pub packet_sender: PacketSender<PacketChannels>,
    pub server_comm_channel: ChannelSender<PacketChannels>,
    pub server_channel: ChannelSender<PacketChannels>,
    pub netplay_channel: ChannelSender<PacketChannels>,
}

impl Connection {
    fn new(
        connection_config: &packets::Config,
        socket_address: SocketAddr,
    ) -> (Self, PacketReceiver<PacketChannels>) {
        let mut connection_builder = ConnectionBuilder::new(connection_config);
        let server_comm_channel =
            connection_builder.bidirectional_channel(PacketChannels::ServerComm);
        let server_channel = connection_builder.sending_channel(PacketChannels::Server);
        let netplay_channel = connection_builder.bidirectional_channel(PacketChannels::Netplay);
        connection_builder.receiving_channel(PacketChannels::Client);

        let (packet_sender, receiver) = connection_builder.build().split();

        let connection = Self {
            socket_address,
            client_id: None,
            netplay_index: 0,
            packet_sender,
            server_comm_channel,
            server_channel,
            netplay_channel,
        };

        (connection, receiver)
    }
}

pub struct PacketOrchestrator {
    socket: Rc<UdpSocket>,
    server_config: Rc<ServerConfig>,
    connection_config: packets::Config,
    connections: SlotMap<slotmap::DefaultKey, Connection>,
    connection_map: HashMap<SocketAddr, slotmap::DefaultKey>,
    client_id_map: HashMap<ActorId, slotmap::DefaultKey>,
    client_room_map: HashMap<SocketAddr, Vec<String>>,
    rooms: HashMap<String, Vec<slotmap::DefaultKey>>,
    netplay_route_map: HashMap<SocketAddr, Vec<SocketAddr>>,
    synchronize_updates: bool,
    synchronize_requests: usize,
    synchronize_locked_clients: HashSet<SocketAddr>,
    pending_receivers: Vec<(SocketAddr, PacketReceiver<PacketChannels>)>,
}

impl PacketOrchestrator {
    pub fn new(socket: Rc<UdpSocket>, server_config: Rc<ServerConfig>) -> PacketOrchestrator {
        let default_config = packets::Config::default();
        let send_budget = server_config.args.send_budget;

        let connection_config = packets::Config {
            mtu: server_config.args.max_payload_size,
            initial_bytes_per_second: default_config.initial_bytes_per_second.min(send_budget),
            max_bytes_per_second: Some(send_budget),
            ..default_config
        };

        PacketOrchestrator {
            socket,
            server_config,
            connection_config,
            connection_map: HashMap::new(),
            connections: Default::default(),
            client_id_map: HashMap::new(),
            client_room_map: HashMap::new(),
            rooms: HashMap::new(),
            netplay_route_map: HashMap::new(),
            synchronize_updates: false,
            synchronize_requests: 0,
            synchronize_locked_clients: HashSet::new(),
            pending_receivers: Vec::new(),
        }
    }

    pub fn request_update_synchronization(&mut self) {
        self.synchronize_updates = true;
        self.synchronize_requests += 1;
    }

    pub fn request_disable_update_synchronization(&mut self) {
        if self.synchronize_requests == 0 {
            log::error!("disable_update_synchronization called too many times!");
            return;
        }

        self.synchronize_requests -= 1;

        if self.synchronize_requests > 0 {
            return;
        }

        let bytes = packets::serialize(&ServerPacket::EndSynchronization);

        for socket_address in &self.synchronize_locked_clients {
            if let Some(index) = self.connection_map.get_mut(socket_address) {
                self.connections[*index]
                    .server_channel
                    .send_bytes(Reliability::ReliableOrdered, &bytes);
            }
        }

        self.synchronize_locked_clients.clear();
        self.synchronize_updates = false;
    }

    pub fn create_connection(&mut self, socket_address: SocketAddr) {
        if self.connection_map.contains_key(&socket_address) {
            return;
        }

        let (connection, receiver) = Connection::new(&self.connection_config, socket_address);
        self.pending_receivers.push((socket_address, receiver));

        let index = self.connections.insert(connection);

        self.connection_map.insert(socket_address, index);
    }

    pub fn take_packet_receivers(&mut self) -> Vec<(SocketAddr, PacketReceiver<PacketChannels>)> {
        std::mem::take(&mut self.pending_receivers)
    }

    pub fn register_client(&mut self, socket_address: SocketAddr, id: ActorId) {
        self.unregister_client(socket_address);

        if let Some(index) = self.connection_map.get(&socket_address) {
            self.connections[*index].client_id = Some(id);
            self.client_id_map.insert(id, *index);
            self.client_room_map.insert(socket_address, Vec::new());
        } else {
            log::error!("Internal Error: Registered client before establishing connection");
        }
    }

    pub fn unregister_client(&mut self, socket_address: SocketAddr) {
        if let Some(index) = self.connection_map.get(&socket_address) {
            let connection = &mut self.connections[*index];

            let Some(id) = &connection.client_id else {
                return;
            };

            self.client_id_map.remove(id);

            let index = connection.netplay_index;
            self.forward_netplay_packet(
                socket_address,
                NetplayPacket::new_disconnect_signal(index),
            );
        }

        // must leave rooms before dropping client_room_map
        if let Some(joined_rooms) = self.client_room_map.get(&socket_address) {
            for room_id in joined_rooms.clone() {
                self.leave_room(socket_address, &room_id);
            }
        }

        // must be dropped after self.leave_room
        self.client_room_map.remove(&socket_address);

        self.disconnect_from_netplay(socket_address);
    }

    pub fn disconnect_from_netplay(&mut self, socket_address: SocketAddr) -> bool {
        let Some(peers) = self.netplay_route_map.remove(&socket_address) else {
            return false;
        };

        for address in &peers {
            if let Some(address_list) = self.netplay_route_map.get_mut(address)
                && let Some(index) = address_list
                    .iter()
                    .position(|address| *address == socket_address)
            {
                address_list.swap_remove(index);
            }
        }

        true
    }

    pub fn drop_connection(&mut self, socket_address: SocketAddr) {
        self.unregister_client(socket_address);

        if let Some(index) = self.connection_map.remove(&socket_address) {
            self.connections.remove(index);
        }
    }

    pub fn configure_netplay_destinations(
        &mut self,
        socket_address: SocketAddr,
        player_index: usize,
        destination_addresses: Vec<SocketAddr>,
    ) {
        if let Some(index) = self.connection_map.get(&socket_address) {
            self.connections[*index].netplay_index = player_index;

            self.netplay_route_map
                .insert(socket_address, destination_addresses);
        }
    }

    pub fn forward_netplay_packet(&self, socket_address: SocketAddr, packet: NetplayPacket) {
        if let Some(index) = self.connection_map.get(&socket_address)
            && self.connections[*index].netplay_index != packet.index
        {
            // client is attempting to impersonate another player, reject the packet
            return;
        }

        let prioritize = packet.prioritize();
        let reliability = packet.default_reliability();
        let data = Arc::new(serialize(packet));

        let Some(addresses) = self.netplay_route_map.get(&socket_address) else {
            return;
        };

        for address in addresses {
            let Some(index) = self.connection_map.get(address) else {
                continue;
            };

            let channel = &self.connections[*index].netplay_channel;

            if prioritize {
                channel.send_shared_bytes_with_priority(reliability, data.clone());
            } else {
                channel.send_shared_bytes(reliability, data.clone());
            }
        }
    }

    pub fn join_room(&mut self, socket_address: SocketAddr, room_id: String) {
        let Some(&connection) = self.connection_map.get(&socket_address) else {
            return;
        };

        let joined_rooms = self.client_room_map.get_mut(&socket_address).unwrap();

        if joined_rooms.contains(&room_id) {
            // already in this room
            return;
        }

        let room = if let Some(room) = self.rooms.get_mut(&room_id) {
            room
        } else {
            // need to create the room
            self.rooms.insert(room_id.to_string(), Vec::new());
            self.rooms.get_mut(&room_id).unwrap()
        };

        room.push(connection);
        joined_rooms.push(room_id.to_string())
    }

    pub fn leave_room(&mut self, socket_address: SocketAddr, room_id: &str) {
        let Some(connection) = self.connection_map.get(&socket_address) else {
            // connection shouldn't be in any rooms anyway if it doesn't exist
            return;
        };

        let joined_rooms = self.client_room_map.get_mut(&socket_address).unwrap();

        if let Some(index) = joined_rooms.iter().position(|id| room_id == id) {
            // drop tracking
            joined_rooms.remove(index);
        } else {
            // not in this room anyway
            return;
        }

        let Some(room) = self.rooms.get_mut(room_id) else {
            // room doesn't exist
            return;
        };

        if let Some(index) = room.iter().position(|i| connection == i) {
            room.remove(index);
        }

        if room.is_empty() {
            // delete empty room
            self.rooms.remove(room_id);
        }
    }

    pub fn send_server_comm(
        &mut self,
        socket_address: SocketAddr,
        reliability: Reliability,
        packet: ServerCommPacket,
    ) {
        if let Some(index) = self.connection_map.get_mut(&socket_address) {
            let connection = &mut self.connections[*index];

            connection
                .server_comm_channel
                .send_serialized(reliability, packet);
        }
    }

    pub fn send(
        &mut self,
        socket_address: SocketAddr,
        reliability: Reliability,
        packet: ServerPacket,
    ) {
        if let Some(index) = self.connection_map.get_mut(&socket_address) {
            let connection = &mut self.connections[*index];

            internal_send_packet(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                packet,
            )
        }
    }

    pub fn send_to_many(
        &mut self,
        socket_addresses: impl IntoIterator<Item = SocketAddr>,
        reliability: Reliability,
        packet: ServerPacket,
    ) {
        let data = Arc::new(serialize(packet));

        for address in socket_addresses.into_iter() {
            if let Some(index) = self.connection_map.get_mut(&address) {
                let connection = &mut self.connections[*index];

                let reliability = handle_synchronization(
                    self.synchronize_updates,
                    &mut self.synchronize_locked_clients,
                    connection,
                    reliability,
                );

                connection
                    .server_channel
                    .send_shared_bytes(reliability, data.clone());
            }
        }
    }

    pub fn send_packets(
        &mut self,
        socket_address: SocketAddr,
        reliability: Reliability,
        packets: Vec<ServerPacket>,
    ) {
        if let Some(index) = self.connection_map.get_mut(&socket_address) {
            let connection = &mut self.connections[*index];

            internal_send_packets(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                packets,
            );
        }
    }

    pub fn send_byte_packets(
        &mut self,
        socket_address: SocketAddr,
        reliability: Reliability,
        packets: &[Vec<u8>],
    ) {
        if let Some(index) = self.connection_map.get_mut(&socket_address) {
            let connection = &mut self.connections[*index];

            internal_send_byte_packets(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                packets,
            );
        }
    }

    pub fn send_by_id(&mut self, id: ActorId, reliability: Reliability, packet: ServerPacket) {
        if let Some(index) = self.client_id_map.get_mut(&id) {
            let connection = &mut self.connections[*index];

            internal_send_packet(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                packet,
            );
        }
    }

    #[allow(dead_code)]
    pub fn send_packets_by_id(
        &mut self,
        id: ActorId,
        reliability: Reliability,
        packets: Vec<ServerPacket>,
    ) {
        if let Some(index) = self.client_id_map.get_mut(&id) {
            let connection = &mut self.connections[*index];

            internal_send_packets(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                packets,
            );
        }
    }

    pub fn send_byte_packets_by_id(
        &mut self,
        id: ActorId,
        reliability: Reliability,
        packets: &[Vec<u8>],
    ) {
        if let Some(index) = self.client_id_map.get_mut(&id) {
            let connection = &mut self.connections[*index];

            internal_send_byte_packets(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                packets,
            );
        }
    }

    pub fn broadcast_to_room(
        &mut self,
        room_id: &str,
        reliability: Reliability,
        packet: ServerPacket,
    ) {
        let Some(room) = self.rooms.get_mut(room_id) else {
            return;
        };

        let bytes = packets::serialize(&packet);

        for index in room {
            let connection = &mut self.connections[*index];

            internal_send_bytes(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                &bytes,
            );
        }
    }

    #[allow(dead_code)]
    pub fn broadcast_packets_to_room(
        &mut self,
        room_id: &str,
        reliability: Reliability,
        packets: Vec<ServerPacket>,
    ) {
        let byte_packets: Vec<Vec<u8>> = packets.iter().map(packets::serialize).collect();

        self.broadcast_byte_packets_to_room(room_id, reliability, &byte_packets)
    }

    pub fn broadcast_byte_packets_to_room(
        &mut self,
        room_id: &str,
        reliability: Reliability,
        packets: &[Vec<u8>],
    ) {
        let Some(room) = self.rooms.get_mut(room_id) else {
            return;
        };

        for index in room {
            let connection = &mut self.connections[*index];

            internal_send_byte_packets(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                packets,
            );
        }
    }

    pub fn broadcast_to_clients(&mut self, reliability: Reliability, packet: ServerPacket) {
        let bytes = packets::serialize(&packet);

        for index in self.client_id_map.values_mut() {
            let connection = &mut self.connections[*index];

            internal_send_bytes(
                self.synchronize_updates,
                &mut self.synchronize_locked_clients,
                connection,
                reliability,
                &bytes,
            );
        }
    }

    pub fn tick(&mut self) -> Vec<Boot> {
        let now = packets::Instant::now();
        let mut kick_list = Vec::new();

        for connection in self.connections.values_mut() {
            connection.packet_sender.tick(now, |bytes| {
                let _ = self.socket.send_to(bytes, connection.socket_address);
            });

            let last_message = connection.packet_sender.last_receive_time();

            if (now - last_message).as_secs_f32() > self.server_config.max_silence_duration {
                kick_list.push(Boot {
                    socket_address: connection.socket_address,
                    reason: String::from("packet silence"),
                    notify_client: true,
                    warp_out: true,
                });
            }
        }

        for boot in &kick_list {
            self.drop_connection(boot.socket_address);
        }

        kick_list
    }
}

// funneling sending packets to these internal functions to handle synchronized updates
fn handle_synchronization(
    synchronize_updates: bool,
    synchronize_locked_clients: &mut HashSet<SocketAddr>,
    connection: &Connection,
    reliability: Reliability,
) -> Reliability {
    if !synchronize_updates || synchronize_locked_clients.contains(&connection.socket_address) {
        return reliability;
    }

    synchronize_locked_clients.insert(connection.socket_address);

    connection.server_channel.send_serialized(
        Reliability::ReliableOrdered,
        ServerPacket::SynchronizeUpdates,
    );

    // force reliable ordered for synchronization
    Reliability::ReliableOrdered
}

fn internal_send_packet(
    synchronize_updates: bool,
    synchronize_locked_clients: &mut HashSet<SocketAddr>,
    connection: &Connection,
    reliability: Reliability,
    packet: ServerPacket,
) {
    let reliability = handle_synchronization(
        synchronize_updates,
        synchronize_locked_clients,
        connection,
        reliability,
    );

    connection
        .server_channel
        .send_serialized(reliability, packet);
}

fn internal_send_bytes(
    synchronize_updates: bool,
    synchronize_locked_clients: &mut HashSet<SocketAddr>,
    connection: &Connection,
    reliability: Reliability,
    bytes: &[u8],
) {
    let reliability = handle_synchronization(
        synchronize_updates,
        synchronize_locked_clients,
        connection,
        reliability,
    );

    connection.server_channel.send_bytes(reliability, bytes);
}

fn internal_send_packets(
    synchronize_updates: bool,
    synchronize_locked_clients: &mut HashSet<SocketAddr>,
    connection: &Connection,
    reliability: Reliability,
    packets: Vec<ServerPacket>,
) {
    let reliability = handle_synchronization(
        synchronize_updates,
        synchronize_locked_clients,
        connection,
        reliability,
    );

    for packet in packets {
        connection
            .server_channel
            .send_serialized(reliability, packet);
    }
}

fn internal_send_byte_packets(
    synchronize_updates: bool,
    synchronize_locked_clients: &mut HashSet<SocketAddr>,
    connection: &Connection,
    reliability: Reliability,
    packets: &[Vec<u8>],
) {
    let reliability = handle_synchronization(
        synchronize_updates,
        synchronize_locked_clients,
        connection,
        reliability,
    );

    for bytes in packets {
        connection.server_channel.send_bytes(reliability, bytes);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_orchestrator() -> PacketOrchestrator {
        let socket = UdpSocket::bind("127.0.0.1:8765").unwrap();
        socket.take_error().unwrap();

        let config = ServerConfig {
            max_silence_duration: 0.0,
            heartbeat_rate: 0.0,
            args: crate::args::Args {
                port: 8765,
                log_connections: false,
                log_packets: false,
                max_payload_size: 1000,
                send_budget: 0,
                receiving_drop_rate: 0.0,
                player_asset_limit: 0,
                avatar_dimensions_limit: 0,
                emotes_animation_path: None,
                emotes_texture_path: None,
            },
        };

        PacketOrchestrator::new(Rc::new(socket), Rc::new(config))
    }

    #[test]
    fn rooms() {
        let mut orchestrator = create_orchestrator();
        let addr: SocketAddr = "127.0.0.1:3000".parse().unwrap();

        let room_a = String::from("A");
        let room_b = String::from("B");
        let room_c = String::from("C");

        orchestrator.join_room(addr, room_c.clone());
        orchestrator.create_connection(addr);
        orchestrator.register_client(addr, ActorId::new(0, 0));
        orchestrator.join_room(addr, room_a.clone());
        orchestrator.join_room(addr, room_b.clone());

        assert!(
            orchestrator.connection_map.contains_key(&addr),
            "connection should exist"
        );

        assert_eq!(
            orchestrator.client_room_map.get(&addr),
            Some(&vec![room_a.clone(), room_b.clone()]),
            "client should only be in room A and B",
        );

        orchestrator.join_room(addr, room_c.clone());
        orchestrator.leave_room(addr, &room_b);

        assert_eq!(
            orchestrator.client_room_map.get(&addr),
            Some(&vec![room_a.clone(), room_c.clone()]),
            "client should no longer be in room B and should be added to room C"
        );

        assert!(
            orchestrator.rooms.contains_key(&room_a),
            "room A should exist"
        );

        assert!(
            !orchestrator.rooms.contains_key(&room_b),
            "room B should not exist"
        );

        assert!(
            orchestrator.rooms.contains_key(&room_c),
            "room C should exist"
        );

        orchestrator.drop_connection(addr);

        assert!(
            !orchestrator.connection_map.contains_key(&addr),
            "connection should not exist"
        );

        assert!(
            !orchestrator.client_room_map.contains_key(&addr),
            "joined_rooms list should not exist"
        );

        assert!(
            !orchestrator.rooms.contains_key(&room_a),
            "room A should not exist"
        );

        assert!(
            !orchestrator.rooms.contains_key(&room_c),
            "room C should not exist"
        );
    }
}
