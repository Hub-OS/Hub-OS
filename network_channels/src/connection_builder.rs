use crate::config::Config;
use crate::packet::{Packet, PacketBuilder};
use crate::packet_sender::PacketSender;
use crate::{ChannelSender, Connection, Label, PacketReceiver};

pub struct ConnectionBuilder<ChannelLabel: Label> {
    config: Config,
    sending_channels: Vec<ChannelLabel>,
    receiving_channels: Vec<ChannelLabel>,
    packet_sender: flume::Sender<PacketBuilder<ChannelLabel>>,
    packet_receiver: flume::Receiver<PacketBuilder<ChannelLabel>>,
}

impl<ChannelLabel: Label> ConnectionBuilder<ChannelLabel> {
    pub fn new(config: &Config) -> Self {
        let (packet_sender, packet_receiver) = flume::unbounded();

        Self {
            config: config.clone(),
            receiving_channels: Vec::new(),
            sending_channels: Vec::new(),
            packet_sender,
            packet_receiver,
        }
    }

    pub fn sending_channel(&mut self, label: ChannelLabel) -> ChannelSender<ChannelLabel> {
        if !self.sending_channels.contains(&label) {
            self.sending_channels.push(label);
        }

        ChannelSender {
            channel: label,
            mtu: self.config.mtu as usize - std::mem::size_of::<Packet<'_, ChannelLabel>>(),
            sender: self.packet_sender.clone(),
        }
    }

    pub fn receiving_channel(&mut self, label: ChannelLabel) {
        if !self.receiving_channels.contains(&label) {
            self.receiving_channels.push(label);
        }
    }

    pub fn bidirectional_channel(&mut self, label: ChannelLabel) -> ChannelSender<ChannelLabel> {
        self.receiving_channel(label);
        self.sending_channel(label)
    }

    pub fn build(self) -> Connection<ChannelLabel> {
        let (ack_sender, ack_receiver) = flume::unbounded();

        let packet_sender = PacketSender::new(
            &self.config,
            &self.sending_channels,
            self.packet_receiver,
            ack_receiver,
        );

        let packet_receiver =
            PacketReceiver::new(&self.receiving_channels, self.packet_sender, ack_sender);

        Connection {
            packet_sender,
            packet_receiver,
        }
    }
}
