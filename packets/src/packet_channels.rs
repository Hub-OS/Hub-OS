use serde::{Deserialize, Serialize};

#[derive(PartialEq, Eq, Clone, Copy, Serialize, Deserialize)]
pub enum PacketChannels {
    Client,
    Server,
    ServerComm,
    PvP,
}
