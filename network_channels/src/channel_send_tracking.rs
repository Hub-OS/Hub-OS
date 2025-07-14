use crate::Reliability;

pub(crate) struct ChannelSendTracking<ChannelLabel> {
    label: ChannelLabel,
    next_unreliable_sequenced: u64,
    next_reliable: u64,
    next_reliable_ordered: u64,
}

impl<ChannelLabel: Copy> ChannelSendTracking<ChannelLabel> {
    pub(crate) fn new(label: ChannelLabel) -> Self {
        Self {
            label,
            next_unreliable_sequenced: 0,
            next_reliable: 0,
            next_reliable_ordered: 0,
        }
    }

    pub(crate) fn label(&self) -> ChannelLabel {
        self.label
    }

    pub(crate) fn peek_next_id(&mut self, reliability: Reliability) -> u64 {
        match reliability {
            Reliability::Unreliable => 0,
            Reliability::UnreliableSequenced => self.next_unreliable_sequenced,
            Reliability::Reliable => self.next_reliable,
            Reliability::ReliableOrdered => self.next_reliable_ordered,
        }
    }

    pub(crate) fn next_id(&mut self, reliability: Reliability) -> u64 {
        match reliability {
            Reliability::Unreliable => 0,
            Reliability::UnreliableSequenced => {
                let id = self.next_unreliable_sequenced;
                self.next_unreliable_sequenced += 1;
                id
            }
            Reliability::Reliable => {
                let id = self.next_reliable;
                self.next_reliable += 1;
                id
            }
            Reliability::ReliableOrdered => {
                let id = self.next_reliable_ordered;
                self.next_reliable_ordered += 1;
                id
            }
        }
    }
}
