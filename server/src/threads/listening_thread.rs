use crate::net::ServerConfig;
use crate::threads::{ListenerMessage, ThreadMessage};
use flume::{Receiver, Sender};
use futures::StreamExt;
use packets::{PacketChannels, deserialize};
use std::time::Instant;
use std::{collections::HashMap, net::SocketAddr};

pub fn create_listening_thread(
    sender: Sender<ThreadMessage>,
    receiver: Receiver<ListenerMessage>,
    socket: std::net::UdpSocket,
    config: ServerConfig,
) {
    let async_socket = smol::net::UdpSocket::try_from(socket).unwrap();

    smol::spawn(listen_loop(sender, receiver, async_socket, config)).detach();
}

async fn listen_loop(
    sender: Sender<ThreadMessage>,
    receiver: Receiver<ListenerMessage>,
    async_socket: smol::net::UdpSocket,
    config: ServerConfig,
) {
    use futures::FutureExt;

    let mut buf = vec![0; config.args.max_payload_size as usize];

    let mut packet_receivers = HashMap::new();

    let mut receiver_stream = receiver.stream();

    loop {
        futures::select_biased! {
            message = receiver_stream.select_next_some() => {
                match message {
                    ListenerMessage::NewConnections { receivers } => {
                        for (address, receiver) in receivers {
                            packet_receivers.insert(address, receiver);
                        }
                    },
                    ListenerMessage::DropConnections { addresses } => {
                        for address in addresses {
                            packet_receivers.remove(&address);
                        }
                    },
                }
            }
            wrapped_packet = async_socket.recv_from(&mut buf).fuse() => {
                if should_drop(config.args.receiving_drop_rate) {
                    // this must come after the recv_from
                    continue;
                }

                let (number_of_bytes, socket_address) = match wrapped_packet {
                    Ok((number_of_bytes, socket_address)) => (number_of_bytes, socket_address),
                    _ => {
                        // don't bring down the whole server over one "connection"
                        continue;
                    }
                };

                let filled_buf = &buf[..number_of_bytes];

                if config.args.log_packets {
                    log::debug!("Received packet from {socket_address}");
                }

                if let Some(packet_receiver) = packet_receivers.get_mut(&socket_address) {
                    match packet_receiver.receive_packet(Instant::now(), filled_buf) {
                        Ok(Some((channel, messages))) => {
                            decode_messages(&sender, socket_address, channel, messages, &config);
                        }
                        Ok(None) => {}
                        Err(e) => {
                            if config.args.log_packets {
                                log::debug!("Failed to decode packet from {socket_address}: {e}");
                            }
                        }
                    };
                } else {
                    sender
                        .send(ThreadMessage::NewConnection { socket_address })
                        .unwrap();
                }
            }
        }
    }
}

fn decode_messages(
    sender: &Sender<ThreadMessage>,
    socket_address: SocketAddr,
    channel: PacketChannels,
    messages: Vec<Vec<u8>>,
    config: &ServerConfig,
) {
    match channel {
        PacketChannels::Client => {
            for message in messages {
                if let Ok(packet) = deserialize(&message) {
                    sender
                        .send(ThreadMessage::ClientPacket {
                            socket_address,
                            packet,
                        })
                        .unwrap();
                }
            }
        }
        PacketChannels::Server => {
            if config.args.log_packets {
                log::debug!("Received unexpected server packet from {socket_address}");
            }
        }
        PacketChannels::ServerComm => {
            for message in messages {
                if let Ok(packet) = deserialize(&message) {
                    sender
                        .send(ThreadMessage::ServerCommPacket {
                            socket_address,
                            packet,
                        })
                        .unwrap();
                }
            }
        }
        PacketChannels::Netplay => {
            for message in messages {
                if let Ok(packet) = deserialize(&message) {
                    sender
                        .send(ThreadMessage::NetplayPacket {
                            socket_address,
                            packet,
                        })
                        .unwrap();
                }
            }
        }
        PacketChannels::SyncData => {
            // this channel is used for syncing data between clients
            // ignored on servers
        }
    }
}

fn should_drop(rate: f32) -> bool {
    if rate == 0.0 {
        return false;
    }

    let roll: f32 = rand::random();

    roll < rate / 100.0
}
