use crate::packet::SenderTask;
use crate::{serialize, Label, Reliability};
use std::sync::mpsc;
use std::sync::Arc;

#[derive(Clone)]
pub struct ChannelSender<ChannelLabel: Label> {
    pub(crate) channel: ChannelLabel,
    pub(crate) mtu: usize,
    pub(crate) sender: mpsc::Sender<SenderTask<ChannelLabel>>,
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

        let fragment_count = data.len().div_ceil(self.mtu) as u64;
        let mut fragment_id = 0;

        while i < data.len() {
            let range = i..data.len().min(i + self.mtu);
            i += self.mtu;

            let _ = self.sender.send(SenderTask::SendMessage {
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
