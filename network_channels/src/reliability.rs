use serde::{Deserialize, Serialize};

#[derive(Debug, PartialEq, Eq, Clone, Copy, Serialize, Deserialize)]
pub enum Reliability {
    Unreliable,
    UnreliableSequenced,
    Reliable,
    // ReliableSequenced,
    ReliableOrdered,
}

impl Reliability {
    pub fn is_reliable(&self) -> bool {
        matches!(self, Reliability::Reliable | Reliability::ReliableOrdered)
    }
}
