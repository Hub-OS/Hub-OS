use crate::resources::MAX_INPUT_DELAY;
use packets::NetplayBufferItem;
use packets::structures::RunLengthDeque;
use serde::{Deserialize, Serialize};

#[derive(Default, Clone, Serialize, Deserialize)]
pub struct PlayerInputBuffer {
    buffer: RunLengthDeque<NetplayBufferItem>,
    len: usize,
    delay: usize,
}

impl PlayerInputBuffer {
    pub fn new_with_delay(delay: usize) -> Self {
        let mut s = Self::default();
        s.set_delay(delay);
        s
    }

    pub fn set_delay(&mut self, mut delay: usize) {
        delay = delay.min(MAX_INPUT_DELAY as usize);

        if delay > 0 {
            self.buffer
                .push_back_many(NetplayBufferItem::default(), delay);
        }

        self.len = delay;
        self.delay = delay;
    }

    pub fn run_length_deque(&self) -> &RunLengthDeque<NetplayBufferItem> {
        &self.buffer
    }

    pub fn append_run_length_deque(&mut self, queue: &RunLengthDeque<NetplayBufferItem>) {
        self.len += queue.len();
        self.buffer.append_clone(queue);
    }

    pub fn delay(&self) -> usize {
        self.delay
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    pub fn push_last(&mut self, input: NetplayBufferItem) {
        self.len += 1;
        self.buffer.push_back(input);
    }

    pub fn delete_last(&mut self) {
        if self.buffer.delete_back() {
            self.len -= 1;
        }
    }

    pub fn peek_next(&self) -> Option<&NetplayBufferItem> {
        self.buffer.peek_next()
    }

    pub fn pop_next(&mut self) -> Option<NetplayBufferItem> {
        let item = self.buffer.pop_front()?;

        self.len -= 1;

        Some(item)
    }

    pub fn get(&self, index: usize) -> Option<&NetplayBufferItem> {
        self.buffer.get(index)
    }

    pub fn iter(&self) -> impl Iterator<Item = &NetplayBufferItem> {
        self.buffer.iter()
    }

    pub fn clear(&mut self) {
        self.buffer.clear();
        self.len = 0;
    }
}
