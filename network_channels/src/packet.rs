use crate::{Instant, Label, Reliability};
use serde::{Deserialize, Serialize};
use std::ops::Range;
use std::sync::Arc;

#[derive(Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub(crate) struct PacketHeader<ChannelLabel> {
    pub channel: ChannelLabel,
    pub reliability: Reliability,
    pub id: u64,
}

#[derive(Serialize, Deserialize)]
pub(crate) enum Packet<'a, ChannelLabel> {
    Message {
        header: PacketHeader<ChannelLabel>,
        fragment_type: FragmentType,
        data: &'a [u8],
    },
    Ack {
        header: PacketHeader<ChannelLabel>,
    },
}

impl<'a, ChannelLabel: Label> Packet<'a, ChannelLabel> {
    pub(crate) fn channel(&self) -> ChannelLabel {
        match self {
            Packet::Message { header, .. } => header.channel,
            Packet::Ack { header } => header.channel,
        }
    }
}

pub(crate) enum PacketBuilder<ChannelLabel> {
    Message {
        channel: ChannelLabel,
        reliability: Reliability,
        fragment_count: u64,
        fragment_id: u64,
        data: Arc<Vec<u8>>,
        range: Range<usize>,
    },
    Ack {
        header: PacketHeader<ChannelLabel>,
        time: Instant,
    },
}

pub(crate) struct Ack<ChannelLabel> {
    pub header: Option<PacketHeader<ChannelLabel>>,
    pub time: Instant,
}

#[derive(Serialize, Deserialize)]
pub(crate) enum FragmentType {
    Full,
    Fragment { id_range: Range<u64> },
}

impl FragmentType {
    // id is the last in the fragment
    pub(crate) fn id_is_tail(&self, id: u64) -> bool {
        match self {
            FragmentType::Full => true,
            FragmentType::Fragment { id_range } => id + 1 == id_range.end,
        }
    }
}
