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
    attempts: u8,
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
    send_trackers: Vec<ChannelSendTracking<ChannelLabel>>,
    task_receiver: mpsc::Receiver<SenderTask<ChannelLabel>>,
    stored_packets: Vec<StoredPacket<ChannelLabel>>,
    last_receive_time: Instant,
    estimated_rtt: Duration,
    retry_delay: Duration,
    rtt_resend_factor: f32,
    mtu: u16,
    bytes_per_sec: usize,
    max_bytes_per_sec: Option<usize>,
    bytes_per_second_increase_factor: f32,
    bytes_per_second_slow_factor: f32,
    remaining_send_budget: usize,
    successfully_sent: usize,
    last_clear: Instant,
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
            send_trackers,
            task_receiver,
            stored_packets: Vec::new(),
            last_receive_time: Instant::now(),
            rtt_resend_factor: config.rtt_resend_factor,
            estimated_rtt: config.initial_rtt,
            retry_delay: config.initial_rtt.mul_f32(config.rtt_resend_factor),
            mtu: config.mtu,
            bytes_per_sec: config.initial_bytes_per_second,
            max_bytes_per_sec: config
                .max_bytes_per_second
                .map(|bytes| bytes.max(config.mtu as usize)),
            bytes_per_second_increase_factor: config.bytes_per_second_increase_factor,
            bytes_per_second_slow_factor: config.bytes_per_second_slow_factor,
            remaining_send_budget: config.initial_bytes_per_second,
            successfully_sent: 0,
            last_clear: Instant::now(),
            #[cfg(feature = "reliable_statistics")]
            statistics: Default::default(),
        }
    }

    /// Last time a packet was received by the [PacketReceiver](crate::PacketReceiver), including Acks. Updates after a [tick](Self::tick).
    pub fn last_receive_time(&self) -> Instant {
        self.last_receive_time
    }

    /// Sends new packets and resends old packets.
    pub fn tick(&mut self, now: Instant, mut send: impl FnMut(&[u8])) {
        // prioritize new packets
        self.send_latest(now, &mut send);
        self.resend_old(now, &mut send);

        if now - self.last_clear > Duration::from_secs(1) {
            // clear successfully sent to avoid speeding up from mostly idling
            self.successfully_sent = 0;
            self.remaining_send_budget = self.bytes_per_sec;
            self.last_clear = now;
        }
    }

    fn send_latest(&mut self, now: Instant, send: &mut dyn FnMut(&[u8])) {
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

                        // try speeding up
                        self.successfully_sent += packet.bytes.len();

                        let threshold = if self.bytes_per_sec > self.mtu as usize * 2 {
                            // must successfully utilize almost all of the budget
                            self.bytes_per_sec - self.mtu as usize * 2
                        } else {
                            // must successfully utilize at least half of the budget
                            self.bytes_per_sec / 2
                        };

                        if self.successfully_sent >= threshold {
                            // speed up
                            self.bytes_per_sec = (self.bytes_per_sec as f32
                                * self.bytes_per_second_increase_factor)
                                as usize;

                            // limit increase
                            if let Some(max) = self.max_bytes_per_sec {
                                self.bytes_per_sec = self.bytes_per_sec.min(max);
                            }

                            self.successfully_sent = 0;
                        }

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

                    let sending = bytes.len() <= self.remaining_send_budget;

                    if sending {
                        self.remaining_send_budget -= bytes.len();
                        send(&bytes);
                    }

                    if reliability.is_reliable() {
                        self.stored_packets.push(StoredPacket {
                            header,
                            bytes,
                            creation: now,
                            next_retry: if sending { now + self.retry_delay } else { now },
                            attempts: 0,
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

                    if bytes.len() <= self.remaining_send_budget {
                        self.remaining_send_budget -= bytes.len();
                        send(&bytes);
                    }
                }
            };
        }
    }

    /// Resends old packets and handles slowing down
    fn resend_old(&mut self, now: Instant, send: &mut dyn FnMut(&[u8])) {
        let mut slowing = false;

        for packet in &mut self.stored_packets {
            if packet.next_retry > now {
                continue;
            }

            if packet.bytes.len() > self.remaining_send_budget {
                // break as soon as we're over budget to avoid busy looping
                break;
            }

            if packet.attempts >= 2 {
                slowing = true;
            }

            packet.attempts += 1;

            self.remaining_send_budget -= packet.bytes.len();
            send(&packet.bytes);
            packet.next_retry = now + self.retry_delay;

            #[cfg(feature = "reliable_statistics")]
            {
                self.statistics.resent += 1;
            }
        }

        if slowing && self.bytes_per_sec > self.mtu as usize {
            // reduce bytes per tick
            self.bytes_per_sec =
                (self.bytes_per_sec as f32 * self.bytes_per_second_slow_factor) as usize;

            // send at least one packet
            self.bytes_per_sec = self.bytes_per_sec.max(self.mtu as _);

            // reset attempts
            for packet in &mut self.stored_packets {
                packet.attempts = 0;
            }

            self.successfully_sent = 0;
        }
    }
}
