use crate::jobs::JobPromise;
use packets::{ClientPacket, PacketChannels, PacketReceiver, ServerCommPacket};
use std::net::SocketAddr;

pub enum ThreadMessage {
    NewConnection {
        socket_address: SocketAddr,
    },
    ServerCommPacket {
        socket_address: SocketAddr,
        packet: ServerCommPacket,
    },
    ClientPacket {
        socket_address: SocketAddr,
        packet: ClientPacket,
    },
    NetplayPacket {
        socket_address: SocketAddr,
        packet: Vec<u8>,
    },
    MessageServer {
        socket_address: SocketAddr,
        data: Vec<u8>,
    },
    PollServer {
        socket_address: SocketAddr,
        promise: JobPromise,
    },
}

pub enum ListenerMessage {
    NewConnections {
        receivers: Vec<(SocketAddr, PacketReceiver<PacketChannels>)>,
    },
    DropConnections {
        addresses: Vec<SocketAddr>,
    },
}
