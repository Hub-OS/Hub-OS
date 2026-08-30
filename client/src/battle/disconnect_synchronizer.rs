use packets::structures::{
    PeerSyncConnectionStates, PeerSyncMessage, PeerSyncResponse, PeerSynchronizer,
};
use std::collections::VecDeque;

pub struct DisconnectSynchronizer {
    synchronizer: Option<PeerSynchronizer<usize, usize>>,
    messages: VecDeque<(usize, PeerSyncMessage<usize>)>,
    outbox: Vec<PeerSyncResponse<usize, usize>>,
}

impl Default for DisconnectSynchronizer {
    fn default() -> Self {
        Self {
            synchronizer: None,
            messages: [(0, PeerSyncMessage::Begin)].into(),
            outbox: Default::default(),
        }
    }
}

impl DisconnectSynchronizer {
    pub fn drain_outbox(&mut self) -> std::vec::Drain<'_, PeerSyncResponse<usize, usize>> {
        self.outbox.drain(..)
    }

    pub fn push_message(&mut self, sender: usize, message: PeerSyncMessage<usize>) {
        self.messages.push_back((sender, message));
    }

    pub fn tick_or_init(
        &mut self,
        local_index: usize,
        connection_states: &impl PeerSyncConnectionStates<usize>,
        init: impl Fn() -> usize,
    ) {
        let synchronizer = self
            .synchronizer
            .get_or_insert_with(|| PeerSynchronizer::new(local_index, init()));

        for (sender_index, message) in self.messages.drain(..) {
            synchronizer.handle_message(
                connection_states,
                local_index,
                sender_index,
                message,
                |response| self.outbox.push(response),
            );
        }
    }
}
