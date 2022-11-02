use crate::args::Args;
use crate::render::FrameTime;
use framework::util::Instant;
use generational_arena::Arena;
use packets::{deserialize, ClientPacket, PacketChannels, PvPPacket, Reliability, ServerPacket};
use std::collections::HashMap;
use std::future::Future;
use std::net::{SocketAddr, UdpSocket};
use std::sync::Arc;
use std::time::Duration;

const DISCONNECT_AFTER: Duration = Duration::from_secs(5);
pub type ClientPacketSender = Arc<dyn Fn(Reliability, ClientPacket) + Send + Sync>;
pub type ServerPacketReceiver = flume::Receiver<ServerPacket>;
pub type PvPPacketSender = Arc<dyn Fn(PvPPacket) + Send + Sync>;
pub type PvPPacketReceiver = flume::Receiver<PvPPacket>;

enum Event {
    ServerSubscription(SocketAddr, flume::Sender<ServerPacket>),
    PvPSubscription(SocketAddr, flume::Sender<PvPPacket>),
    SendingClientPacket(SocketAddr, Reliability, ClientPacket),
    SendingPvPPacket(SocketAddr, PvPPacket),
    ReceivedPacket(SocketAddr, Instant, Vec<u8>),
    Tick,
}

pub struct Network {
    socket: Arc<UdpSocket>,
    sender: flume::Sender<Event>,
    time: FrameTime,
}

impl Network {
    pub fn new(args: &Args) -> Self {
        let (sender, receiver) = flume::unbounded();

        let socket = UdpSocket::bind(&format!("0.0.0.0:{}", args.port)).unwrap();
        let socket = Arc::new(socket);

        std::thread::spawn({
            let sender = sender.clone();
            let socket = socket.clone();

            move || socket_listener(socket, sender)
        });

        std::thread::spawn({
            let listener = EventListener::new(socket.clone(), receiver, args.resend_budget);

            move || listener.run()
        });

        Self {
            socket,
            sender,
            time: 0,
        }
    }

    pub fn subscribe_to_pvp(
        &self,
        address: String,
    ) -> impl Future<Output = Option<(PvPPacketSender, PvPPacketReceiver)>> {
        let event_sender = self.sender.clone();

        async move {
            let addr = packets::address_parsing::resolve_socket_addr(address.as_str()).await?;

            let (sender, receiver) = flume::unbounded();

            event_sender
                .send(Event::PvPSubscription(addr, sender))
                .ok()?;

            let send_packet: PvPPacketSender = Arc::new(move |body| {
                let _ = event_sender.send(Event::SendingPvPPacket(addr, body));
            });

            Some((send_packet, receiver))
        }
    }

    pub fn subscribe_to_server(
        &self,
        address: String,
    ) -> impl Future<Output = Option<(ClientPacketSender, ServerPacketReceiver)>> {
        let event_sender = self.sender.clone();

        async move {
            let addr = packets::address_parsing::resolve_socket_addr(address.as_str()).await?;

            let (sender, receiver) = flume::unbounded();

            event_sender
                .send(Event::ServerSubscription(addr, sender))
                .ok()?;

            let send_packet: ClientPacketSender = Arc::new(move |reliability, body| {
                let _ = event_sender.send(Event::SendingClientPacket(addr, reliability, body));
            });

            Some((send_packet, receiver))
        }
    }

    pub fn tick(&mut self) {
        self.time += 1;

        // run 20 times per second
        if self.time % (60 / 20) == 0 {
            self.sender.send(Event::Tick).unwrap();
        }
    }
}

fn socket_listener(socket: Arc<UdpSocket>, packet_sender: flume::Sender<Event>) {
    let mut buffer = Vec::new();
    buffer.resize(100000, 0);

    while !packet_sender.is_disconnected() {
        let (addr, bytes) = match socket.recv_from(&mut buffer) {
            Ok((len, addr)) => (addr, &buffer[..len]),
            _ => continue,
        };

        let _ = packet_sender.send(Event::ReceivedPacket(addr, Instant::now(), bytes.to_vec()));
    }
}

struct Connection {
    socket_addr: SocketAddr,
    client_channel: packets::ChannelSender<PacketChannels>,
    pvp_channel: packets::ChannelSender<PacketChannels>,
    packet_sender: packets::PacketSender<PacketChannels>,
    packet_receiver: packets::PacketReceiver<PacketChannels>,
    server_subscribers: Vec<flume::Sender<ServerPacket>>,
    pvp_subscribers: Vec<flume::Sender<PvPPacket>>,
    stored_pvp_packets: Vec<PvPPacket>,
}

impl Connection {
    fn new(socket_addr: SocketAddr) -> Self {
        let mut builder = packets::ConnectionBuilder::new(&packets::Config::default());
        builder.receiving_channel(PacketChannels::Server);
        let client_channel = builder.sending_channel(PacketChannels::Client);
        let pvp_channel = builder.bidirectional_channel(PacketChannels::PvP);
        let (packet_sender, packet_receiver) = builder.build().split();

        Self {
            socket_addr,
            client_channel,
            pvp_channel,
            packet_sender,
            packet_receiver,
            server_subscribers: Vec::new(),
            pvp_subscribers: Vec::new(),
            stored_pvp_packets: Vec::new(),
        }
    }
}

struct EventListener {
    socket: Arc<UdpSocket>,
    connection_map: HashMap<SocketAddr, generational_arena::Index>,
    connections: Arena<Connection>,
    receiver: flume::Receiver<Event>,
    resend_budget: usize,
}

impl EventListener {
    fn new(socket: Arc<UdpSocket>, receiver: flume::Receiver<Event>, resend_budget: usize) -> Self {
        Self {
            socket,
            connection_map: HashMap::new(),
            connections: Arena::new(),
            receiver,
            resend_budget,
        }
    }

    fn run(mut self) {
        loop {
            let event = match self.receiver.recv() {
                Ok(event) => event,
                _ => return,
            };

            match event {
                Event::ServerSubscription(addr, sender) => {
                    self.handle_server_subscription(addr, sender)
                }
                Event::PvPSubscription(addr, sender) => self.handle_pvp_subscription(addr, sender),
                Event::SendingClientPacket(addr, reliability, body) => {
                    self.send_client_packet(addr, reliability, packets::serialize(body))
                }
                Event::SendingPvPPacket(addr, body) => self.send_pvp_packet(
                    addr,
                    Reliability::ReliableOrdered,
                    packets::serialize(body),
                ),
                Event::ReceivedPacket(addr, time, body) => self.sort_packet(addr, time, body),
                Event::Tick => self.tick(),
            }
        }
    }

    fn handle_server_subscription(
        &mut self,
        addr: SocketAddr,
        sender: flume::Sender<ServerPacket>,
    ) {
        // create a connection if it doesnt already exist

        if let Some(index) = self.connection_map.get_mut(&addr) {
            self.connections[*index].server_subscribers.push(sender);
        } else {
            let mut connection = Connection::new(addr);
            connection.server_subscribers.push(sender);
            let index = self.connections.insert(connection);
            self.connection_map.insert(addr, index);
        }
    }

    fn handle_pvp_subscription(&mut self, addr: SocketAddr, sender: flume::Sender<PvPPacket>) {
        // create a connection if it doesnt already exist

        if let Some(index) = self.connection_map.get_mut(&addr) {
            let connection = &mut self.connections[*index];

            // push stored pvp packets
            for packet in std::mem::take(&mut connection.stored_pvp_packets) {
                let _ = sender.send(packet);
            }

            connection.pvp_subscribers.push(sender);
        } else {
            let mut connection = Connection::new(addr);
            connection.pvp_subscribers.push(sender);
            let index = self.connections.insert(connection);
            self.connection_map.insert(addr, index);
        }
    }

    fn send_client_packet(&mut self, addr: SocketAddr, reliability: Reliability, bytes: Vec<u8>) {
        let connection = match self.connection_map.get_mut(&addr) {
            Some(index) => &mut self.connections[*index],
            None => return,
        };

        connection
            .client_channel
            .send_shared_bytes(reliability, Arc::new(bytes));

        // push asap
        connection.packet_sender.tick(Instant::now(), |bytes| {
            let _ = self.socket.send_to(bytes, connection.socket_addr);
        })
    }

    fn send_pvp_packet(&mut self, addr: SocketAddr, reliability: Reliability, bytes: Vec<u8>) {
        let connection = match self.connection_map.get_mut(&addr) {
            Some(index) => &mut self.connections[*index],
            None => return,
        };

        connection
            .pvp_channel
            .send_shared_bytes(reliability, Arc::new(bytes));

        // push asap
        connection.packet_sender.tick(Instant::now(), |bytes| {
            let _ = self.socket.send_to(bytes, connection.socket_addr);
        })
    }

    fn sort_packet(&mut self, addr: SocketAddr, time: Instant, bytes: Vec<u8>) {
        let connection = match self.connection_map.get_mut(&addr) {
            Some(index) => &mut self.connections[*index],
            None => return,
        };

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
                log::warn!("Received Client packet from remote, expecting server or pvp packets");
            }
            PacketChannels::ServerComm => {
                log::warn!(
                    "Received ServerComm packet from remote, expecting server or pvp packets"
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
            PacketChannels::PvP => {
                let mut pending_pvp_removal = Vec::new();

                for message in messages {
                    let pvp_packet: PvPPacket = match deserialize(&message) {
                        Ok(pvp_packet) => pvp_packet,
                        Err(e) => {
                            log::error!(
                                "Failed to deserialize pvp packet from {}: {e}",
                                connection.socket_addr
                            );
                            continue;
                        }
                    };

                    for (i, sender) in connection.pvp_subscribers.iter().enumerate() {
                        if sender.send(pvp_packet.clone()).is_err()
                            && !pending_pvp_removal.contains(&i)
                        {
                            pending_pvp_removal.push(i);
                        }
                    }

                    if connection.pvp_subscribers.is_empty()
                        || pending_pvp_removal.len() == connection.pvp_subscribers.len()
                    {
                        // if no one can receive this packet, store it for new subscribers
                        // this is to handle a case where packets are routed through the server
                        // and the remote client is closer, causing packets to possibly be received before we enter a pvp scene
                        connection.stored_pvp_packets.push(pvp_packet);
                    }
                }

                for i in pending_pvp_removal.into_iter().rev() {
                    connection.pvp_subscribers.remove(i);
                }
            }
        }
    }

    fn tick(&mut self) {
        let now = Instant::now();

        self.handle_disconnections(now);
        self.send_packets(now);
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
        }

        // remove dead subscribers
        for (_, connection) in &mut self.connections {
            let pending_removal: Vec<_> = connection
                .server_subscribers
                .iter()
                .enumerate()
                .filter(|(_, subscriber)| subscriber.is_disconnected())
                .map(|(i, _)| i)
                .collect();

            for index in pending_removal.into_iter().rev() {
                connection.server_subscribers.swap_remove(index);
            }
        }
    }

    fn send_packets(&mut self, now: Instant) {
        for (_, connection) in &mut self.connections {
            connection.packet_sender.tick(now, |bytes| {
                let _ = self.socket.send_to(bytes, connection.socket_addr);
            });
        }
    }
}
