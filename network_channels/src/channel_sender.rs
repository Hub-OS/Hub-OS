use crate::packet::SenderTask;
use crate::{serialize, Label, Reliability};
use std::sync::mpsc;
use std::sync::Arc;

#[derive(Clone)]
pub struct ChannelSender<ChannelLabel: Label> {
    pub(crate) channel: ChannelLabel,
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
        self.internal_send(false, reliability, data);
    }

    pub fn send_serialized_with_priority(
        &self,
        reliability: Reliability,
        data: impl serde::Serialize,
    ) {
        let data = Arc::new(serialize(data));
        self.send_shared_bytes_with_priority(reliability, data);
    }

    pub fn send_bytes_with_priority(&self, reliability: Reliability, data: &[u8]) {
        self.send_shared_bytes_with_priority(reliability, Arc::new(data.to_vec()));
    }

    pub fn send_shared_bytes_with_priority(&self, reliability: Reliability, data: Arc<Vec<u8>>) {
        self.internal_send(true, reliability, data);
    }

    fn internal_send(&self, priority: bool, reliability: Reliability, data: Arc<Vec<u8>>) {
        let _ = self.sender.send(SenderTask::SendMessage {
            channel: self.channel,
            priority,
            reliability,
            data,
        });
    }
}
