use crate::channel_send_tracking::ChannelSendTracking;
use crate::config::Config;
use crate::packet::{FragmentType, Packet, PacketHeader, SenderTask};
use crate::{Label, serialize};
use instant::{Duration, Instant};
use std::sync::mpsc;

pub struct StoredPacket<ChannelLabel> {
    header: PacketHeader<ChannelLabel>,
    bytes: Vec<u8>,
    send_time: Option<Instant>,
    next_retry: Instant,
    attempts: u8,
    priority: bool,
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
    rtt_resend_factor: f32,
    mtu: u16,
    bytes_per_sec: f32,
    max_bytes_per_sec: Option<usize>,
    bytes_per_second_increase_factor: f32,
    bytes_per_second_slow_factor: f32,
    remaining_send_budget: usize,
    successfully_sent: usize,
    last_tick: Instant,
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

        let now = Instant::now();

        Self {
            send_trackers,
            task_receiver,
            stored_packets: Vec::new(),
            last_receive_time: now,
            rtt_resend_factor: config.rtt_resend_factor,
            estimated_rtt: config.initial_rtt,
            mtu: config.mtu,
            bytes_per_sec: config.initial_bytes_per_second as _,
            max_bytes_per_sec: config
                .max_bytes_per_second
                .map(|bytes| bytes.max(config.mtu as usize)),
            bytes_per_second_increase_factor: config.bytes_per_second_increase_factor,
            bytes_per_second_slow_factor: config.bytes_per_second_slow_factor,
            remaining_send_budget: config.initial_bytes_per_second,
            successfully_sent: 0,
            last_tick: now,
            last_clear: now,
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
        let restored_budget = (now - self.last_tick).as_secs_f32() * self.bytes_per_sec;
        self.remaining_send_budget = self
            .remaining_send_budget
            .saturating_add(restored_budget as _)
            .min(self.bytes_per_sec as _);

        // prioritize new packets
        self.send_latest(now, &mut send);
        self.resend_old(now, &mut send);

        if now - self.last_clear > self.estimated_rtt * 2 {
            // clear successfully sent to avoid speeding up from mostly idling
            self.successfully_sent = 0;
            self.last_clear = now;
        }

        self.last_tick = now;
    }

    fn send_latest(&mut self, now: Instant, send: &mut dyn FnMut(&[u8])) {
        while let Ok(packet_instruction) = self.task_receiver.try_recv() {
            match packet_instruction {
                SenderTask::Ack { header, time } => {
                    self.last_receive_time = time;

                    let mut packets_iter = self.stored_packets.iter();

                    if let Some(index) = packets_iter.position(|packet| packet.header == header) {
                        let packet = self.stored_packets.remove(index);
                        self.sample_ack(time, packet);

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
                    priority,
                    reliability,
                    data,
                } => {
                    let Some(tracker) = self
                        .send_trackers
                        .iter_mut()
                        .find(|tracker| tracker.label() == channel)
                    else {
                        continue;
                    };

                    let chunk_size =
                        self.mtu as usize - std::mem::size_of::<Packet<'_, ChannelLabel>>();

                    let fragment_type = if data.len() <= chunk_size {
                        FragmentType::Full
                    } else {
                        if !reliability.is_reliable() {
                            // don't fragment if this is an unreliable packet
                            // just drop it
                            continue;
                        }

                        let total_fragments = data.len().div_ceil(chunk_size);
                        let start = tracker.peek_next_id(reliability);
                        let end = start + total_fragments as u64;

                        FragmentType::Fragment {
                            id_range: start..end,
                        }
                    };

                    for chunk in data.chunks(chunk_size) {
                        let header = PacketHeader {
                            channel,
                            reliability,
                            id: tracker.next_id(reliability),
                        };

                        let bytes = serialize(&Packet::Message {
                            header,
                            fragment_type: fragment_type.clone(),
                            data: chunk,
                        });

                        let sending = bytes.len() <= self.remaining_send_budget || priority;

                        if sending {
                            send(&bytes);

                            self.remaining_send_budget =
                                self.remaining_send_budget.saturating_sub(bytes.len());

                            if !reliability.is_reliable() {
                                // count unreliable packets as successfully sent to avoid blocking speed increases
                                self.successfully_sent += bytes.len();
                            }
                        }

                        if reliability.is_reliable() {
                            let mut stored_packet = StoredPacket {
                                header,
                                bytes,
                                send_time: None,
                                next_retry: now,
                                attempts: 0,
                                priority,
                            };

                            if sending {
                                let retry_delay =
                                    self.estimated_rtt.mul_f32(self.rtt_resend_factor);
                                stored_packet.next_retry = now + retry_delay;
                                stored_packet.attempts = 1
                            }

                            if priority {
                                self.stored_packets.insert(0, stored_packet);
                            } else {
                                self.stored_packets.push(stored_packet);
                            }

                            #[cfg(feature = "reliable_statistics")]
                            {
                                self.statistics.sent += 1;
                            }
                        }
                    }
                }
                SenderTask::SendAck { header, time } => {
                    self.last_receive_time = time;
                    let bytes = serialize(&Packet::Ack { header });

                    self.remaining_send_budget =
                        self.remaining_send_budget.saturating_sub(bytes.len());

                    // count acks as successfully sent to avoid blocking speed increases
                    self.successfully_sent += bytes.len();

                    // send acks even if we're over send budget
                    send(&bytes);
                }
            };
        }
    }

    fn sample_ack(&mut self, time: Instant, acked_packet: StoredPacket<ChannelLabel>) {
        let Some(send_time) = acked_packet.send_time else {
            return;
        };

        // resolve rtt using a smoothing factor, matching TCP
        const A: f32 = 0.125;
        const A_FLIPPED: f32 = 1.0 - A;

        let rtt_sample = time - send_time;
        self.estimated_rtt = self.estimated_rtt.mul_f32(A_FLIPPED) + rtt_sample.mul_f32(A);

        // try speeding up
        self.successfully_sent += acked_packet.bytes.len();

        let max_bytes_per_rtt = self.estimated_rtt.as_secs_f32() * self.bytes_per_sec;
        let threshold = max_bytes_per_rtt as usize;

        if self.successfully_sent >= threshold {
            // speed up
            self.bytes_per_sec *= self.bytes_per_second_increase_factor;

            // limit increase
            if let Some(max) = self.max_bytes_per_sec {
                self.bytes_per_sec = self.bytes_per_sec.min(max as _);
            }

            self.successfully_sent = 0;
        }
    }

    /// Resends old packets and handles slowing down
    fn resend_old(&mut self, now: Instant, send: &mut dyn FnMut(&[u8])) {
        let mut slowing = false;

        let retry_delay = self.estimated_rtt.mul_f32(self.rtt_resend_factor);

        let scaled_attempts = (3.0 / self.rtt_resend_factor).ceil();
        let max_attempts_before_slow = (scaled_attempts.clamp(0.0, u8::MAX as _) as u8).max(3);

        for packet in &mut self.stored_packets {
            if packet.next_retry > now {
                continue;
            }

            if !packet.priority && packet.bytes.len() > self.remaining_send_budget {
                // break as soon as we're over budget to avoid busy looping
                break;
            }

            if packet.attempts >= max_attempts_before_slow {
                slowing = true;
                // avoid sending more packets
                // as we're possibly flooding the network and need to slow down
                break;
            }

            packet.attempts += 1;

            self.remaining_send_budget = self
                .remaining_send_budget
                .saturating_sub(packet.bytes.len());

            send(&packet.bytes);
            packet.next_retry = now + retry_delay;

            if packet.send_time.is_none() {
                packet.send_time = Some(now);
            }

            #[cfg(feature = "reliable_statistics")]
            {
                self.statistics.resent += 1;
            }
        }

        if slowing {
            // reduce bytes per tick
            self.bytes_per_sec *= self.bytes_per_second_slow_factor;

            // send at least one packet
            self.bytes_per_sec = self.bytes_per_sec.max(self.mtu as _);

            // reset attempts
            for packet in &mut self.stored_packets {
                packet.attempts = 0;
            }

            self.successfully_sent = 0;
            self.remaining_send_budget = 0;
        }
    }
}
