use crate::channel_send_tracking::ChannelSendTracking;
use crate::config::Config;
use crate::packet::{Ack, FragmentType, Packet, PacketBuilder, PacketHeader};
use crate::{serialize, Label};
use instant::{Duration, Instant};

pub struct StoredPacket<ChannelLabel> {
    header: PacketHeader<ChannelLabel>,
    bytes: Vec<u8>,
    creation: Instant,
    next_retry: Instant,
}

pub struct PacketSender<ChannelLabel: Label> {
    bytes_per_tick: usize,
    send_trackers: Vec<ChannelSendTracking<ChannelLabel>>,
    packet_receiver: flume::Receiver<PacketBuilder<ChannelLabel>>,
    ack_receiver: flume::Receiver<Ack<ChannelLabel>>,
    stored_packets: Vec<StoredPacket<ChannelLabel>>,
    last_receive_time: Instant,
    rtt_resend_factor: f32,
    retry_delay: Duration,
}

impl<ChannelLabel: Label> PacketSender<ChannelLabel> {
    pub(crate) fn new(
        config: &Config,
        channels: &[ChannelLabel],
        packet_receiver: flume::Receiver<PacketBuilder<ChannelLabel>>,
        ack_receiver: flume::Receiver<Ack<ChannelLabel>>,
    ) -> Self {
        let send_trackers: Vec<_> = channels
            .iter()
            .map(|label| ChannelSendTracking::new(*label))
            .collect();

        Self {
            bytes_per_tick: config.bytes_per_tick,
            send_trackers,
            packet_receiver,
            ack_receiver,
            stored_packets: Vec::new(),
            last_receive_time: Instant::now(),
            rtt_resend_factor: config.rtt_resend_factor,
            retry_delay: config.initial_rtt.mul_f32(config.rtt_resend_factor),
        }
    }

    /// Last time a packet was received by the PacketReceiver, including Acks. Updates after a tick()
    pub fn last_receive_time(&self) -> Instant {
        self.last_receive_time
    }

    /// Sends pending packets including internally generated packets such as Acks, updates last_receive_time
    pub fn tick(&mut self, now: Instant, send: impl Fn(&[u8])) {
        while let Ok(ack) = self.ack_receiver.try_recv() {
            self.last_receive_time = ack.time;

            if let Some(header) = ack.header {
                if let Some(index) = self
                    .stored_packets
                    .iter()
                    .position(|packet| packet.header == header)
                {
                    let packet = self.stored_packets.remove(index);

                    let rtt = ack.time - packet.creation;
                    let updated_retry_delay = rtt.mul_f32(self.rtt_resend_factor);
                    self.retry_delay = self.retry_delay.min(updated_retry_delay);
                }
            }
        }

        let mut budget = self.bytes_per_tick;

        while let Ok(packet_instruction) = self.packet_receiver.try_recv() {
            match &packet_instruction {
                PacketBuilder::Message {
                    channel,
                    reliability,
                    fragment_count,
                    fragment_id,
                    data,
                    range,
                } => {
                    let tracker = match self
                        .send_trackers
                        .iter_mut()
                        .find(|tracker| tracker.label() == *channel)
                    {
                        Some(tracker) => tracker,
                        None => continue,
                    };

                    let header = PacketHeader {
                        channel: *channel,
                        reliability: *reliability,
                        id: tracker.next_id(*reliability),
                    };

                    let bytes = serialize(&Packet::Message {
                        header,
                        fragment_type: if *fragment_count == 1 {
                            FragmentType::Full
                        } else {
                            let start = header.id - fragment_id;
                            let end = start + *fragment_count;

                            FragmentType::Fragment {
                                id_range: start..end,
                            }
                        },
                        data: &data[range.start..range.end],
                    });

                    let sending = bytes.len() <= budget;

                    if sending {
                        budget -= bytes.len();
                        send(&bytes);
                    }

                    if reliability.is_reliable() {
                        self.stored_packets.push(StoredPacket {
                            header,
                            bytes,
                            creation: now,
                            next_retry: if sending { now + self.retry_delay } else { now },
                        })
                    }
                }
                PacketBuilder::Ack { header, time } => {
                    self.last_receive_time = *time;
                    let bytes = serialize(&Packet::Ack { header: *header });

                    if bytes.len() <= budget {
                        budget -= bytes.len();
                        send(&bytes);
                    }
                }
            };

            for packet in &mut self.stored_packets {
                if packet.next_retry <= now && packet.bytes.len() <= budget {
                    budget -= packet.bytes.len();
                    send(&packet.bytes);
                    packet.next_retry = now + self.retry_delay;
                }
            }
        }
    }
}
