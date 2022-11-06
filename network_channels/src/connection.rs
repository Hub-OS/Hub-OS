use crate::packet_sender::PacketSender;
use crate::{DecodeError, Instant, Label, PacketReceiver};

pub struct Connection<ChannelLabel: Label> {
    pub(crate) packet_sender: PacketSender<ChannelLabel>,
    pub(crate) packet_receiver: PacketReceiver<ChannelLabel>,
}

impl<ChannelLabel: Label> Connection<ChannelLabel> {
    pub fn split(self) -> (PacketSender<ChannelLabel>, PacketReceiver<ChannelLabel>) {
        (self.packet_sender, self.packet_receiver)
    }

    pub fn tick(&mut self, now: Instant, send: impl Fn(&[u8])) {
        self.packet_sender.tick(now, send);
    }

    #[allow(clippy::type_complexity)]
    pub fn receive_packet(
        &mut self,
        now: Instant,
        data: &[u8],
    ) -> Result<Option<(ChannelLabel, Vec<Vec<u8>>)>, DecodeError> {
        self.packet_receiver.receive_packet(now, data)
    }
}
