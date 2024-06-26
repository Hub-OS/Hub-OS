use crate::packet::{FragmentType, PacketHeader};
use crate::*;
use std::ops::Range;

struct BackedUpPacket {
    pub id: u64,
    pub fragment_type: FragmentType,
    pub body: Vec<u8>,
}

struct FragmentCollector {
    id_range: Range<u64>,
    fragments: Vec<(u64, Vec<u8>)>,
}

pub(crate) struct ChannelReceiver<ChannelLabel> {
    channel: ChannelLabel,
    next_reliable: u64,
    next_unreliable_sequenced: u64,
    next_reliable_ordered: u64,
    missing_reliable: Vec<u64>,
    backed_up_ordered_packets: Vec<BackedUpPacket>,
    fragment_collectors: Vec<FragmentCollector>,
}

impl<ChannelLabel: Copy> ChannelReceiver<ChannelLabel> {
    pub(crate) fn new(channel: ChannelLabel) -> Self {
        Self {
            channel,
            next_reliable: 0,
            next_unreliable_sequenced: 0,
            next_reliable_ordered: 0,
            missing_reliable: Vec::new(),
            backed_up_ordered_packets: Vec::new(),
            fragment_collectors: Vec::new(),
        }
    }

    pub(crate) fn channel(&self) -> ChannelLabel {
        self.channel
    }

    fn resolve_fragment(
        &mut self,
        header: PacketHeader<ChannelLabel>,
        fragment_type: FragmentType,
        body: Vec<u8>,
    ) -> Option<Vec<u8>> {
        let id_range = match fragment_type {
            FragmentType::Fragment { id_range } => id_range,
            FragmentType::Full => return Some(body),
        };

        let collector_index = self
            .fragment_collectors
            .iter()
            .position(|collector| collector.id_range == id_range);

        let Some(collector_index) = collector_index else {
            // create a new collector
            let collector = FragmentCollector {
                id_range,
                fragments: vec![(header.id, body)],
            };

            self.fragment_collectors.push(collector);
            return None;
        };

        let collector = &mut self.fragment_collectors[collector_index];
        let expected_len = id_range.end - id_range.start;

        collector.fragments.push((header.id, body));

        if expected_len != collector.fragments.len() as u64 {
            // still missing some fragments
            return None;
        }

        // we can form the message
        collector.fragments.sort_by_key(|(id, _)| *id);

        let collector = self.fragment_collectors.remove(collector_index);
        let data = collector
            .fragments
            .into_iter()
            .flat_map(|(_, body)| body.into_iter())
            .collect();

        Some(data)
    }

    pub(crate) fn sort_packet(
        &mut self,
        header: PacketHeader<ChannelLabel>,
        fragment_type: FragmentType,
        body: Vec<u8>,
    ) -> Vec<Vec<u8>> {
        match header.reliability {
            Reliability::Unreliable => vec![body],
            Reliability::UnreliableSequenced => {
                if header.id < self.next_unreliable_sequenced {
                    // ignore old packets
                    vec![]
                } else {
                    self.next_unreliable_sequenced = header.id + 1;
                    vec![body]
                }
            }
            Reliability::Reliable => {
                if header.id == self.next_reliable {
                    // expected
                    self.next_reliable += 1;
                } else if header.id > self.next_reliable {
                    // skipped expected
                    self.missing_reliable.extend(self.next_reliable..header.id);
                    self.next_reliable = header.id + 1;
                } else if let Some(i) = self.missing_reliable.iter().position(|id| *id == header.id)
                {
                    // one of the missing packets
                    self.missing_reliable.remove(i);
                } else {
                    // we already handled this packet
                    return vec![];
                }

                match self.resolve_fragment(header, fragment_type, body) {
                    Some(message) => vec![message],
                    None => vec![],
                }
            }
            Reliability::ReliableOrdered => {
                if header.id < self.next_reliable_ordered {
                    // already handled
                    return vec![];
                }

                let new_id = header.id;

                self.insert_reliable(header, fragment_type, body);

                let first_id = self.backed_up_ordered_packets[0].id;
                let skip = (new_id - first_id) as usize;

                let mut i = skip;
                let mut tail_i = None;

                // find the furthest sequential tail
                for backed_up_packet in self.backed_up_ordered_packets.iter().skip(skip) {
                    let id = backed_up_packet.id;

                    if id > self.next_reliable_ordered {
                        break;
                    }

                    if id != self.next_reliable_ordered {
                        continue;
                    }

                    if backed_up_packet.fragment_type.id_is_tail(id) {
                        tail_i = Some(i);
                    }

                    self.next_reliable_ordered += 1;

                    i += 1;
                }

                let Some(tail_i) = tail_i else {
                    // no tail, we only have incomplete fragments
                    return vec![];
                };

                // split backed up packets where the gap is
                let mut newer_packets = self.backed_up_ordered_packets.split_off(tail_i + 1);

                // store the packets waiting for the gap
                std::mem::swap(&mut self.backed_up_ordered_packets, &mut newer_packets);

                // merge fragments
                let mut packets = vec![vec![]];

                for packet in newer_packets {
                    match packet.fragment_type {
                        FragmentType::Full => {
                            *packets.last_mut().unwrap() = packet.body;
                            packets.push(vec![]);
                        }
                        FragmentType::Fragment { .. } => {
                            packets.last_mut().unwrap().extend(packet.body);

                            if packet.fragment_type.id_is_tail(packet.id) {
                                packets.push(vec![]);
                            }
                        }
                    }
                }

                packets.pop();

                packets
            }
        }
    }

    fn insert_reliable(
        &mut self,
        header: PacketHeader<ChannelLabel>,
        fragment_type: FragmentType,
        body: Vec<u8>,
    ) {
        // sorted insert
        let mut i = 0;
        let mut should_insert = true;

        for backed_up_packet in &self.backed_up_ordered_packets {
            if backed_up_packet.id == header.id {
                should_insert = false;
                break;
            }
            if backed_up_packet.id > header.id {
                break;
            }
            i += 1;
        }

        if should_insert {
            self.backed_up_ordered_packets.insert(
                i,
                BackedUpPacket {
                    id: header.id,
                    fragment_type,
                    body,
                },
            );
        }
    }
}
