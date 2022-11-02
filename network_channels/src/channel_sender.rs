use std::sync::Arc;

use crate::packet::PacketBuilder;
use crate::{serialize, Label, Reliability};

#[derive(Clone)]
pub struct ChannelSender<ChannelLabel: Label> {
    pub(crate) channel: ChannelLabel,
    pub(crate) mtu: usize,
    pub(crate) sender: flume::Sender<PacketBuilder<ChannelLabel>>,
}

impl<ChannelLabel: Label> ChannelSender<ChannelLabel> {
    pub fn send_serialized(&self, reliability: Reliability, data: impl serde::Serialize) {
        let data = Arc::new(serialize(data));
        self.send_shared_bytes(reliability, data);
    }

    pub fn send_bytes(&self, reliability: Reliability, data: &[u8]) {
        self.send_shared_bytes(reliability, Arc::new(data.to_vec()));
    }

    pub fn send_shared_bytes(&self, reliability: Reliability, data: Arc<Vec<u8>>) {
        if !reliability.is_reliable() && data.len() > self.mtu {
            // fragmentation is not supported for unreliable packets
            // these packets are unreliable, so safe to silently drop
            return;
        }

        let mut i = 0;

        let fragment_count = ((data.len() + self.mtu - 1) / self.mtu) as u64;
        let mut fragment_id = 0;

        while i < data.len() {
            let range = i..data.len().min(i + self.mtu);
            i += self.mtu;

            let _ = self.sender.send(PacketBuilder::Message {
                channel: self.channel,
                reliability,
                fragment_count,
                fragment_id,
                data: data.clone(),
                range,
            });

            fragment_id += 1;
        }
    }
}
