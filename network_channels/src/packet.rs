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

impl<ChannelLabel: Label> Packet<'_, ChannelLabel> {
    pub(crate) fn channel(&self) -> ChannelLabel {
        match self {
            Packet::Message { header, .. } => header.channel,
            Packet::Ack { header } => header.channel,
        }
    }
}

pub(crate) enum SenderTask<ChannelLabel> {
    SendMessage {
        channel: ChannelLabel,
        priority: bool,
        reliability: Reliability,
        data: Arc<Vec<u8>>,
    },
    SendAck {
        header: PacketHeader<ChannelLabel>,
        time: Instant,
    },
    Ack {
        header: PacketHeader<ChannelLabel>,
        time: Instant,
    },
}

#[derive(Clone, Serialize, Deserialize)]
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
