use crate::channel_send_tracking::ChannelSendTracking;
use crate::config::Config;
use crate::packet::{FragmentType, Packet, PacketHeader, SenderTask};
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
    task_receiver: mpsc::Receiver<SenderTask<ChannelLabel>>,
    stored_packets: Vec<StoredPacket<ChannelLabel>>,
    last_receive_time: Instant,
    estimated_rtt: Duration,
    retry_delay: Duration,
    rtt_resend_factor: f32,
    #[cfg(feature = "reliable_statistics")]
    statistics: ReliablePacketStatistics,
}

impl<ChannelLabel: Label> PacketSender<ChannelLabel> {
    pub(crate) fn new(
        config: &Config,
        channels: &[ChannelLabel],
        task_receiver: mpsc::Receiver<SenderTask<ChannelLabel>>,
    ) -> Self {
        let send_trackers: Vec<_> = channels
            .iter()
            .map(|label| ChannelSendTracking::new(*label))
            .collect();

        Self {
            bytes_per_tick: config.bytes_per_tick,
            send_trackers,
            task_receiver,
            stored_packets: Vec::new(),
            last_receive_time: Instant::now(),
            rtt_resend_factor: config.rtt_resend_factor,
            estimated_rtt: config.initial_rtt,
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
        let mut budget = self.bytes_per_tick;

        while let Ok(packet_instruction) = self.task_receiver.try_recv() {
            match packet_instruction {
                SenderTask::Ack { header, time } => {
                    self.last_receive_time = time;

                    let Some(header) = header else {
                        continue;
                    };

                    let mut packets_iter = self.stored_packets.iter();

                    if let Some(index) = packets_iter.position(|packet| packet.header == header) {
                        let packet = self.stored_packets.remove(index);

                        // resolve rtt using a smoothing factor, matching TCP
                        const A: f32 = 0.125;
                        const A_FLIPPED: f32 = 1.0 - A;

                        let rtt_sample = time - packet.creation;
                        self.estimated_rtt =
                            self.estimated_rtt.mul_f32(A_FLIPPED) + rtt_sample.mul_f32(A);

                        // update retry delay
                        let new_delay = self.estimated_rtt.mul_f32(self.rtt_resend_factor);
                        self.retry_delay = self.retry_delay.min(new_delay);

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
                SenderTask::SendMessage {
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
                        .find(|tracker| tracker.label() == channel)
                    else {
                        continue;
                    };

                    let header = PacketHeader {
                        channel,
                        reliability,
                        id: tracker.next_id(reliability),
                    };

                    let bytes = serialize(&Packet::Message {
                        header,
                        fragment_type: if fragment_count == 1 {
                            FragmentType::Full
                        } else {
                            let start = header.id - fragment_id;
                            let end = start + fragment_count;

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
                SenderTask::SendAck { header, time } => {
                    self.last_receive_time = time;
                    let bytes = serialize(&Packet::Ack { header });

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
