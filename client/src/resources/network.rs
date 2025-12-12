use crate::args::Args;
use crate::structures::{DenseSlotMap, GenerationalIndex};
use framework::math::Instant;
use framework::prelude::async_sleep;
use futures::StreamExt;
use packets::{
    ClientPacket, MULTICAST_ADDR, MULTICAST_PORT, MULTICAST_V4, MulticastPacket,
    MulticastPacketWrapper, NetplayPacket, PacketChannels, Reliability, SERVER_TICK_RATE,
    SERVER_TICK_RATE_F, ServerPacket, SyncDataPacket, deserialize,
};
use std::collections::HashMap;
use std::future::Future;
use std::net::{Ipv4Addr, SocketAddr, UdpSocket};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, OnceLock};
use std::time::Duration;

pub type ClientPacketSender = Arc<dyn Fn(Reliability, ClientPacket) + Send + Sync>;
pub type ServerPacketReceiver = flume::Receiver<ServerPacket>;
pub type NetplayPacketSender = Arc<dyn Fn(NetplayPacket) + Send + Sync>;
pub type NetplayPacketReceiver = flume::Receiver<NetplayPacket>;
pub type SyncDataPacketSender = Arc<dyn Fn(Reliability, SyncDataPacket) + Send + Sync>;
pub type SyncDataPacketReceiver = flume::Receiver<SyncDataPacket>;
pub type MulticastReceiver = flume::Receiver<(SocketAddr, MulticastPacket)>;

const DISCONNECT_AFTER: Duration = Duration::from_secs(5);
const TICK_RATE: Duration = SERVER_TICK_RATE;
const TPS: usize = (1.0 / SERVER_TICK_RATE_F) as _;

#[derive(PartialEq, Eq, Clone, Copy)]
pub enum ServerStatus {
    Online,
    Offline,
    TooOld,
    TooNew,
    Incompatible,
    InvalidAddress,
}

enum Event {
    ServerSubscription(SocketAddr, flume::Sender<ServerPacket>),
    NetplaySubscription(SocketAddr, flume::Sender<NetplayPacketReceiver>),
    SyncDataSubscription(SocketAddr, flume::Sender<SyncDataPacketReceiver>),
    MulticastSubscription(flume::Sender<(SocketAddr, MulticastPacket)>),
    SendingClientPacket(SocketAddr, Reliability, ClientPacket),
    SendingNetplayPacket(SocketAddr, NetplayPacket),
    SendingSyncPacket(SocketAddr, Reliability, SyncDataPacket),
    SendingMulticastPacket(MulticastPacket),
    ReceivedPacket(SocketAddr, Instant, Vec<u8>),
    ReceivedMulticastPacket(SocketAddr, MulticastPacket, usize),
}

#[derive(Default)]
pub struct NetworkDetails {
    local_address: OnceLock<String>,
    per_sec_up: AtomicUsize,
    per_sec_down: AtomicUsize,
    total_up: AtomicUsize,
    total_down: AtomicUsize,
}

impl NetworkDetails {
    // Resolved when outgoing multicast packets are read
    pub fn local_address(&self) -> Option<&str> {
        self.local_address.get().map(|s| s.as_str())
    }

    pub fn per_sec_bytes_up(&self) -> usize {
        self.per_sec_up.load(Ordering::Relaxed)
    }

    pub fn per_sec_bytes_down(&self) -> usize {
        self.per_sec_down.load(Ordering::Relaxed)
    }

    pub fn total_bytes_up(&self) -> usize {
        self.total_up.load(Ordering::Relaxed)
    }

    pub fn total_bytes_down(&self) -> usize {
        self.total_down.load(Ordering::Relaxed)
    }
}

pub struct Network {
    socket: Arc<UdpSocket>,
    sender: flume::Sender<Event>,
    network_details: Arc<NetworkDetails>,
}

impl Network {
    pub fn new(args: &Args) -> Self {
        let (sender, receiver) = flume::unbounded();

        let network_details = Arc::new(NetworkDetails::default());

        // multicast listener
        std::thread::spawn({
            let sender = sender.clone();
            let network_details = network_details.clone();

            move || multicast_listener(sender, network_details)
        });

        // regular socket
        let socket = UdpSocket::bind(format!("0.0.0.0:{}", args.port)).unwrap();
        let socket = Arc::new(socket);

        std::thread::spawn({
            let sender = sender.clone();
            let socket = socket.clone();

            move || socket_listener(socket, sender)
        });

        std::thread::spawn({
            let network_details = network_details.clone();
            let listener =
                EventListener::new(socket.clone(), receiver, args.send_budget, network_details);

            move || listener.run()
        });

        Self {
            socket,
            sender,
            network_details,
        }
    }

    // Resolved when outgoing multicast packets are read
    pub fn details(&self) -> &NetworkDetails {
        &self.network_details
    }

    pub fn subscribe_to_netplay(
        &self,
        address: String,
    ) -> impl Future<Output = Option<(NetplayPacketSender, NetplayPacketReceiver)>> + use<> {
        let event_sender = self.sender.clone();

        async move {
            let addr = packets::address_parsing::resolve_socket_addr(address.as_str()).await?;

            let (sender, receiver) = flume::bounded(1);

            event_sender
                .send(Event::NetplaySubscription(addr, sender))
                .ok()?;

            let send_packet: NetplayPacketSender = Arc::new(move |body| {
                let _ = event_sender.send(Event::SendingNetplayPacket(addr, body));
            });

            let receiver = receiver.recv_async().await.ok()?;

            Some((send_packet, receiver))
        }
    }

    pub fn subscribe_to_server(
        &self,
        address: String,
    ) -> impl Future<Output = Option<(ClientPacketSender, ServerPacketReceiver)>> + use<> {
        let event_sender = self.sender.clone();

        async move {
            let addr = packets::address_parsing::resolve_socket_addr(address.as_str()).await?;

            let (sender, receiver) = flume::unbounded();

            event_sender
                .send(Event::ServerSubscription(addr, sender))
                .ok()?;

            let send: ClientPacketSender = Arc::new(move |reliability, body| {
                let _ = event_sender.send(Event::SendingClientPacket(addr, reliability, body));
            });

            Some((send, receiver))
        }
    }

    pub fn subscribe_to_sync_data(
        &self,
        address: String,
    ) -> impl Future<Output = Option<(SyncDataPacketSender, SyncDataPacketReceiver)>> + use<> {
        let event_sender = self.sender.clone();

        async move {
            let addr = packets::address_parsing::resolve_socket_addr(address.as_str()).await?;

            let (sender, receiver) = flume::unbounded();

            event_sender
                .send(Event::SyncDataSubscription(addr, sender))
                .ok()?;

            let send_packet: SyncDataPacketSender = Arc::new(move |reliability, body| {
                let _ = event_sender.send(Event::SendingSyncPacket(addr, reliability, body));
            });

            let receiver = receiver.recv_async().await.ok()?;

            Some((send_packet, receiver))
        }
    }

    pub fn subscribe_to_multicast(&self) -> MulticastReceiver {
        let (sender, receiver) = flume::unbounded();
        let _ = self.sender.send(Event::MulticastSubscription(sender));
        receiver
    }

    pub fn send_multicast(&self, packet: MulticastPacket) {
        let _ = self.sender.send(Event::SendingMulticastPacket(packet));
    }

    pub async fn poll_server(
        send: &ClientPacketSender,
        receiver: &ServerPacketReceiver,
    ) -> ServerStatus {
        while !receiver.is_disconnected() {
            send(Reliability::Unreliable, ClientPacket::VersionRequest);

            async_sleep(TICK_RATE).await;

            let response = match receiver.try_recv() {
                Ok(response) => response,
                _ => continue,
            };

            let (version_id, version_iteration) = match response {
                ServerPacket::VersionInfo {
                    version_id,
                    version_iteration,
                } => (version_id, version_iteration),
                ServerPacket::Kick { .. } => {
                    return ServerStatus::Offline;
                }
                _ => {
                    continue;
                }
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
    }
}

fn socket_listener(socket: Arc<UdpSocket>, packet_sender: flume::Sender<Event>) {
    let mut buffer = vec![0; 100000];

    while !packet_sender.is_disconnected() {
        let (addr, bytes) = match socket.recv_from(&mut buffer) {
            Ok((len, addr)) => (addr, &buffer[..len]),
            _ => continue,
        };

        let _ = packet_sender.send(Event::ReceivedPacket(addr, Instant::now(), bytes.to_vec()));
    }
}

fn multicast_listener(packet_sender: flume::Sender<Event>, network_details: Arc<NetworkDetails>) {
    let socket = match UdpSocket::bind(format!("0.0.0.0:{MULTICAST_PORT}")) {
        Ok(socket) => socket,
        Err(err) => {
            log::warn!("Failed to join multicast group for data sync: {err:?}");
            return;
        }
    };

    if let Err(err) = socket.join_multicast_v4(&MULTICAST_V4, &Ipv4Addr::UNSPECIFIED) {
        log::warn!("Failed to join multicast group for data sync: {err:?}");
        return;
    }

    let mut buffer = vec![0; 100000];
    let mut local_address_set = false;

    while !packet_sender.is_disconnected() {
        let (addr, bytes) = match socket.recv_from(&mut buffer) {
            Ok((len, addr)) => (addr, &buffer[..len]),
            _ => continue,
        };

        if let Ok(wrapped_packet) = packets::deserialize::<MulticastPacketWrapper>(bytes) {
            if wrapped_packet.is_compatible() {
                let event = Event::ReceivedMulticastPacket(addr, wrapped_packet.data, bytes.len());
                let _ = packet_sender.send(event);
            } else if !local_address_set && wrapped_packet.uuid == MulticastPacket::local_uuid() {
                let _ = network_details.local_address.set(addr.to_string());
                local_address_set = true;
            }
        }
    }
}

struct Connection {
    socket_addr: SocketAddr,
    client_channel: packets::ChannelSender<PacketChannels>,
    netplay_channel: packets::ChannelSender<PacketChannels>,
    sync_channel: packets::ChannelSender<PacketChannels>,
    packet_sender: packets::PacketSender<PacketChannels>,
    packet_receiver: packets::PacketReceiver<PacketChannels>,
    server_subscribers: Vec<flume::Sender<ServerPacket>>,
    netplay_sender: flume::Sender<NetplayPacket>,
    netplay_recycled_receiver: NetplayPacketReceiver,
}

impl Connection {
    fn new(socket_addr: SocketAddr, send_budget: usize) -> Self {
        let default_config = packets::Config::default();

        let mut builder = packets::ConnectionBuilder::new(&packets::Config {
            initial_bytes_per_second: default_config.initial_bytes_per_second.min(send_budget),
            max_bytes_per_second: Some(send_budget),
            ..Default::default()
        });
        builder.receiving_channel(PacketChannels::Server);
        let client_channel = builder.sending_channel(PacketChannels::Client);
        let netplay_channel = builder.bidirectional_channel(PacketChannels::Netplay);
        let sync_channel = builder.bidirectional_channel(PacketChannels::SyncData);
        let (packet_sender, packet_receiver) = builder.build().split();
        let (netplay_sender, netplay_recycled_receiver) = flume::unbounded();

        Self {
            socket_addr,
            client_channel,
            netplay_channel,
            sync_channel,
            packet_sender,
            packet_receiver,
            server_subscribers: Vec::new(),
            netplay_sender,
            netplay_recycled_receiver,
        }
    }
}

#[derive(Default)]
struct NetworkSample {
    bytes_up: usize,
    bytes_down: usize,
}

struct EventListener {
    socket: Arc<UdpSocket>,
    connection_map: HashMap<SocketAddr, GenerationalIndex>,
    connections: DenseSlotMap<Connection>,
    sync_senders_and_receivers: HashMap<
        GenerationalIndex,
        (
            flume::Sender<SyncDataPacket>,
            flume::Receiver<SyncDataPacket>,
        ),
    >,
    multicast_listeners: Vec<flume::Sender<(SocketAddr, MulticastPacket)>>,
    receiver: flume::Receiver<Event>,
    network_details: Arc<NetworkDetails>,
    network_samples: [NetworkSample; TPS],
    next_network_sample: usize,
    bytes_up: usize,
    bytes_down: usize,
    send_budget: usize,
}

impl EventListener {
    fn new(
        socket: Arc<UdpSocket>,
        receiver: flume::Receiver<Event>,
        send_budget: usize,
        network_details: Arc<NetworkDetails>,
    ) -> Self {
        Self {
            socket,
            connection_map: HashMap::new(),
            connections: Default::default(),
            sync_senders_and_receivers: Default::default(),
            multicast_listeners: Default::default(),
            receiver,
            send_budget,
            network_details,
            network_samples: Default::default(),
            next_network_sample: 0,
            bytes_up: 0,
            bytes_down: 0,
        }
    }

    fn run(mut self) {
        smol::block_on(async {
            let tick_stream = smol::Timer::interval(TICK_RATE).fuse();
            let receiver_clone = self.receiver.clone();
            let receiver_stream = receiver_clone.stream().fuse();

            futures::pin_mut!(tick_stream, receiver_stream);

            loop {
                futures::select! {
                    _ = tick_stream.select_next_some() => {
                        if self.receiver.is_disconnected() {
                            return;
                        }

                        self.tick();
                    }
                    event = receiver_stream.select_next_some() => {
                        self.handle_event(event);
                    }
                }
            }
        })
    }

    fn handle_event(&mut self, event: Event) {
        match event {
            Event::ServerSubscription(addr, sender) => {
                self.handle_server_subscription(addr, sender)
            }
            Event::NetplaySubscription(addr, sender) => {
                self.handle_netplay_subscription(addr, sender)
            }
            Event::SyncDataSubscription(addr, sender) => {
                self.handle_sync_data_subscription(addr, sender)
            }
            Event::MulticastSubscription(sender) => {
                self.multicast_listeners.push(sender);
            }
            Event::SendingClientPacket(addr, reliability, body) => {
                self.send_client_packet(addr, reliability, body)
            }
            Event::SendingNetplayPacket(addr, body) => {
                self.send_netplay_packet(addr, body.default_reliability(), body)
            }
            Event::SendingSyncPacket(addr, reliability, body) => {
                self.send_sync_packet(addr, reliability, body)
            }
            Event::SendingMulticastPacket(body) => {
                let bytes = packets::serialize(MulticastPacketWrapper::new(body));
                self.bytes_up += bytes.len();
                let _ = self.socket.send_to(&bytes, MULTICAST_ADDR);
            }
            Event::ReceivedPacket(addr, time, body) => {
                self.bytes_down += body.len();
                self.sort_packet(addr, time, body);
            }
            Event::ReceivedMulticastPacket(addr, body, len) => {
                for sender in &self.multicast_listeners {
                    let _ = sender.send((addr, body.clone()));
                }
                self.bytes_down += len;
            }
        }
    }

    fn get_or_init_connection(&mut self, addr: SocketAddr) -> GenerationalIndex {
        if let Some(index) = self.connection_map.get_mut(&addr) {
            *index
        } else {
            let connection = Connection::new(addr, self.send_budget);
            let index = self.connections.insert(connection);
            self.connection_map.insert(addr, index);
            index
        }
    }

    fn handle_server_subscription(
        &mut self,
        addr: SocketAddr,
        sender: flume::Sender<ServerPacket>,
    ) {
        let index = self.get_or_init_connection(addr);
        let connection = &mut self.connections[index];
        connection.server_subscribers.push(sender)
    }

    fn handle_netplay_subscription(
        &mut self,
        addr: SocketAddr,
        sender: flume::Sender<NetplayPacketReceiver>,
    ) {
        let index = self.get_or_init_connection(addr);
        let connection = &mut self.connections[index];
        let _ = sender.send(connection.netplay_recycled_receiver.clone());
    }

    fn handle_sync_data_subscription(
        &mut self,
        addr: SocketAddr,
        sender: flume::Sender<SyncDataPacketReceiver>,
    ) {
        let index = self.get_or_init_connection(addr);
        let (_, packet_receiver) = self
            .sync_senders_and_receivers
            .entry(index)
            .or_insert_with(flume::unbounded);

        let _ = sender.send(packet_receiver.clone());
    }

    fn send_client_packet(
        &mut self,
        addr: SocketAddr,
        reliability: Reliability,
        packet: ClientPacket,
    ) {
        let Some(index) = self.connection_map.get_mut(&addr) else {
            return;
        };
        let connection = &mut self.connections[*index];

        connection
            .client_channel
            .send_serialized(reliability, packet);

        // push asap
        connection.packet_sender.tick(Instant::now(), |bytes| {
            self.bytes_up += bytes.len();
            let _ = self.socket.send_to(bytes, connection.socket_addr);
        })
    }

    fn send_netplay_packet(
        &mut self,
        addr: SocketAddr,
        reliability: Reliability,
        packet: NetplayPacket,
    ) {
        let Some(index) = self.connection_map.get_mut(&addr) else {
            return;
        };
        let connection = &mut self.connections[*index];

        if packet.prioritize() {
            connection
                .netplay_channel
                .send_serialized_with_priority(reliability, packet);
        } else {
            connection
                .netplay_channel
                .send_serialized(reliability, packet);
        }

        // push asap
        connection.packet_sender.tick(Instant::now(), |bytes| {
            self.bytes_up += bytes.len();
            let _ = self.socket.send_to(bytes, connection.socket_addr);
        })
    }

    fn send_sync_packet(
        &mut self,
        addr: SocketAddr,
        reliability: Reliability,
        packet: SyncDataPacket,
    ) {
        let Some(index) = self.connection_map.get_mut(&addr) else {
            return;
        };
        let connection = &mut self.connections[*index];

        connection.sync_channel.send_serialized(reliability, packet);

        // push asap
        connection.packet_sender.tick(Instant::now(), |bytes| {
            self.bytes_up += bytes.len();
            let _ = self.socket.send_to(bytes, connection.socket_addr);
        })
    }

    fn sort_packet(&mut self, addr: SocketAddr, time: Instant, bytes: Vec<u8>) {
        let Some(index) = self.connection_map.get_mut(&addr) else {
            return;
        };
        let connection = &mut self.connections[*index];

        let (channel, messages) = match connection.packet_receiver.receive_packet(time, &bytes) {
            Ok(Some((channel, messages))) => (channel, messages),
            Ok(None) => {
                return;
            }
            Err(e) => {
                log::error!(
                    "Failed to deserialize packet from {}: {e}",
                    connection.socket_addr
                );
                return;
            }
        };

        // similar branches, maybe these could be condensed into structs?
        match channel {
            PacketChannels::Client => {
                log::warn!(
                    "Received Client packet from remote, expecting server or netplay packets"
                );
            }
            PacketChannels::ServerComm => {
                log::warn!(
                    "Received ServerComm packet from remote, expecting server or netplay packets"
                );
            }
            PacketChannels::Server => {
                let mut pending_server_removal = Vec::new();

                for message in messages {
                    let server_packet: ServerPacket = match deserialize(&message) {
                        Ok(server_packet) => server_packet,
                        Err(e) => {
                            log::error!(
                                "Failed to deserialize server packet from {}: {e}",
                                connection.socket_addr
                            );
                            continue;
                        }
                    };

                    for (i, sender) in connection.server_subscribers.iter().enumerate() {
                        if sender.send(server_packet.clone()).is_err()
                            && !pending_server_removal.contains(&i)
                        {
                            pending_server_removal.push(i);
                        }
                    }
                }

                for i in pending_server_removal.into_iter().rev() {
                    connection.server_subscribers.remove(i);
                }
            }
            PacketChannels::Netplay => {
                for message in messages {
                    let netplay_packet: NetplayPacket = match deserialize(&message) {
                        Ok(netplay_packet) => netplay_packet,
                        Err(e) => {
                            log::error!(
                                "Failed to deserialize netplay packet from {}: {e}",
                                connection.socket_addr
                            );
                            continue;
                        }
                    };

                    let _ = connection.netplay_sender.send(netplay_packet);
                }
            }
            PacketChannels::SyncData => {
                if let Some((sender, _)) = self.sync_senders_and_receivers.get(index) {
                    for message in messages {
                        let sync_packet: SyncDataPacket = match deserialize(&message) {
                            Ok(packet) => packet,
                            Err(e) => {
                                log::error!(
                                    "Failed to deserialize data sync packet from {}: {e}",
                                    connection.socket_addr
                                );
                                continue;
                            }
                        };

                        let _ = sender.send(sync_packet);
                    }
                }
            }
        }
    }

    fn tick(&mut self) {
        let now = Instant::now();

        self.handle_disconnections(now);
        self.send_packets(now);

        // update total up and down
        let details = &self.network_details;
        let ordering = Ordering::Relaxed;
        details.total_up.fetch_add(self.bytes_up, ordering);
        details.total_down.fetch_add(self.bytes_down, ordering);

        // update network samples
        let current_sample = &mut self.network_samples[self.next_network_sample];
        current_sample.bytes_up = self.bytes_up;
        current_sample.bytes_down = self.bytes_down;
        self.bytes_up = 0;
        self.bytes_down = 0;

        self.next_network_sample += 1;
        self.next_network_sample %= TPS;

        // update bytes per sec
        let mut up = 0;
        let mut down = 0;

        for sample in &self.network_samples {
            up += sample.bytes_up;
            down += sample.bytes_down;
        }

        details.per_sec_up.store(up, ordering);
        details.per_sec_down.store(down, ordering);
    }

    fn handle_disconnections(&mut self, now: Instant) {
        let disconnected: Vec<_> = self
            .connections
            .iter()
            .map(|(_, connection)| connection)
            .filter(|connection| {
                now - connection.packet_receiver.last_receive_time() >= DISCONNECT_AFTER
            })
            .map(|connection| connection.socket_addr)
            .collect();

        for addr in disconnected.into_iter().rev() {
            let index = self.connection_map.remove(&addr).unwrap();
            self.connections.remove(index);
            self.sync_senders_and_receivers.remove(&index);
        }

        // remove dead subscribers
        for (_, connection) in &mut self.connections {
            connection
                .server_subscribers
                .retain(|subscriber| !subscriber.is_disconnected());
        }

        self.multicast_listeners
            .retain(|subscriber| !subscriber.is_disconnected());
    }

    fn send_packets(&mut self, now: Instant) {
        for (_, connection) in &mut self.connections {
            connection.packet_sender.tick(now, |bytes| {
                self.bytes_up += bytes.len();
                let _ = self.socket.send_to(bytes, connection.socket_addr);
            });
        }
    }
}
