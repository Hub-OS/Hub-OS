use crate::config::Config;
use crate::packet::SenderTask;
use crate::packet_sender::PacketSender;
use crate::{ChannelSender, Connection, Label, PacketReceiver};
use std::sync::mpsc;

pub struct ConnectionBuilder<ChannelLabel: Label> {
    config: Config,
    sending_channels: Vec<ChannelLabel>,
    receiving_channels: Vec<ChannelLabel>,
    task_sender: mpsc::Sender<SenderTask<ChannelLabel>>,
    task_receiver: mpsc::Receiver<SenderTask<ChannelLabel>>,
}

impl<ChannelLabel: Label> ConnectionBuilder<ChannelLabel> {
    pub fn new(config: &Config) -> Self {
        let (task_sender, task_receiver) = mpsc::channel();

        Self {
            config: config.clone(),
            receiving_channels: Vec::new(),
            sending_channels: Vec::new(),
            task_sender,
            task_receiver,
        }
    }

    pub fn sending_channel(&mut self, label: ChannelLabel) -> ChannelSender<ChannelLabel> {
        if !self.sending_channels.contains(&label) {
            self.sending_channels.push(label);
        }

        ChannelSender {
            channel: label,
            sender: self.task_sender.clone(),
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
        let packet_sender =
            PacketSender::new(&self.config, &self.sending_channels, self.task_receiver);

        let packet_receiver = PacketReceiver::new(&self.receiving_channels, self.task_sender);

        Connection {
            packet_sender,
            packet_receiver,
        }
    }
}
