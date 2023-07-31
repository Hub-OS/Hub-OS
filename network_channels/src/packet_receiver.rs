use crate::channel_receiver::ChannelReceiver;
use crate::deserialize;
use crate::packet::{Ack, Packet, PacketBuilder};
use crate::{DecodeError, Instant, Label};

pub type ReceiveResult<T> = std::result::Result<T, DecodeError>;

pub struct PacketReceiver<ChannelLabel> {
    channel_receivers: Vec<ChannelReceiver<ChannelLabel>>,
    packet_sender: flume::Sender<PacketBuilder<ChannelLabel>>,
    ack_sender: flume::Sender<Ack<ChannelLabel>>,
    last_receive_time: Instant,
}

impl<ChannelLabel: Label> PacketReceiver<ChannelLabel> {
    pub(crate) fn new(
        channels: &[ChannelLabel],
        packet_sender: flume::Sender<PacketBuilder<ChannelLabel>>,
        ack_sender: flume::Sender<Ack<ChannelLabel>>,
    ) -> Self {
        let channel_receivers: Vec<_> = channels
            .iter()
            .map(|channel| ChannelReceiver::new(*channel))
            .collect();

        Self {
            channel_receivers,
            packet_sender,
            ack_sender,
            last_receive_time: Instant::now(),
        }
    }

    pub fn last_receive_time(&self) -> Instant {
        self.last_receive_time
    }

    pub fn receive_packet<'a>(
        &mut self,
        now: Instant,
        data: &'a [u8],
    ) -> ReceiveResult<Option<(ChannelLabel, Vec<Vec<u8>>)>> {
        self.last_receive_time = now;

        let packet: Packet<'a, ChannelLabel> = deserialize(data)?;

        let channel = packet.channel();

        match packet {
            Packet::Message {
                header,
                fragment_type,
                data,
            } => {
                if header.reliability.is_reliable() {
                    let _ = self
                        .packet_sender
                        .send(PacketBuilder::Ack { header, time: now });
                }

                if let Some(receiver) = self
                    .channel_receivers
                    .iter_mut()
                    .find(|r| r.channel() == header.channel)
                {
                    return Ok(Some((
                        channel,
                        receiver.sort_packet(header, fragment_type, data.to_vec()),
                    )));
                }

                let _ = self.ack_sender.send(Ack {
                    header: None,
                    time: now,
                });
            }
            Packet::Ack { header } => {
                let _ = self.ack_sender.send(Ack {
                    header: Some(header),
                    time: now,
                });
            }
        }

        Ok(None)
    }
}
