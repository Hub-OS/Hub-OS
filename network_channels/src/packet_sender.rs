use crate::channel_send_tracking::ChannelSendTracking;
use crate::config::Config;
use crate::packet::{Ack, FragmentType, Packet, PacketBuilder, PacketHeader};
use crate::{serialize, Label};
use instant::{Duration, Instant};
use std::sync::mpsc;

pub struct StoredPacket<ChannelLabel> {
    header: PacketHeader<ChannelLabel>,
    bytes: Vec<u8>,
    creation: Instant,
    next_retry: Instant,
}

#[cfg(feature = "reliable_statistics")]
pub struct ReliablePacketStatistics {
    pub sent: usize,
    pub resent: usize,
    pub acknowledged: usize,
    pub redundant: usize,
}

#[cfg(feature = "reliable_statistics")]
impl ReliablePacketStatistics {
    fn unaccounted() -> usize {
        self.sent + self.resent - self.acknowledged - self.redundant
    }
}

pub struct PacketSender<ChannelLabel: Label> {
    bytes_per_tick: usize,
    send_trackers: Vec<ChannelSendTracking<ChannelLabel>>,
    packet_receiver: mpsc::Receiver<PacketBuilder<ChannelLabel>>,
    ack_receiver: mpsc::Receiver<Ack<ChannelLabel>>,
    stored_packets: Vec<StoredPacket<ChannelLabel>>,
    last_receive_time: Instant,
    rtt_resend_factor: f32,
    retry_delay: Duration,
    #[cfg(feature = "reliable_statistics")]
    statistics: ReliablePacketStatistics,
}

impl<ChannelLabel: Label> PacketSender<ChannelLabel> {
    pub(crate) fn new(
        config: &Config,
        channels: &[ChannelLabel],
        packet_receiver: mpsc::Receiver<PacketBuilder<ChannelLabel>>,
        ack_receiver: mpsc::Receiver<Ack<ChannelLabel>>,
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
            #[cfg(feature = "reliable_statistics")]
            statistics: Default::default(),
        }
    }

    /// Last time a packet was received by the PacketReceiver, including Acks. Updates after a tick()
    pub fn last_receive_time(&self) -> Instant {
        self.last_receive_time
    }

    /// Sends pending packets including internally generated packets such as Acks, updates last_receive_time
    pub fn tick(&mut self, now: Instant, mut send: impl FnMut(&[u8])) {
        while let Ok(ack) = self.ack_receiver.try_recv() {
            self.last_receive_time = ack.time;

            let Some(header) = ack.header else {
                continue;
            };

            let mut packets_iter = self.stored_packets.iter();

            if let Some(index) = packets_iter.position(|packet| packet.header == header) {
                let packet = self.stored_packets.remove(index);

                let rtt = ack.time - packet.creation;
                let updated_retry_delay = rtt.mul_f32(self.rtt_resend_factor);
                self.retry_delay = self.retry_delay.min(updated_retry_delay);

                #[cfg(feature = "reliable_statistics")]
                {
                    self.statistics.acknowledged += 1;
                }
            } else {
                #[cfg(feature = "reliable_statistics")]
                {
                    self.statistics.redundant += 1;
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
                    let Some(tracker) = self
                        .send_trackers
                        .iter_mut()
                        .find(|tracker| tracker.label() == *channel)
                    else {
                        continue;
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
                        });

                        #[cfg(feature = "reliable_statistics")]
                        {
                            self.statistics.sent += 1;
                        }
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
        }

        for packet in &mut self.stored_packets {
            if packet.next_retry > now {
                continue;
            }

            if packet.bytes.len() > budget {
                // break as soon as we're over budget to avoid busy looping
                break;
            }

            budget -= packet.bytes.len();
            send(&packet.bytes);
            packet.next_retry = now + self.retry_delay;

            #[cfg(feature = "reliable_statistics")]
            {
                self.statistics.resent += 1;
            }
        }
    }
}
