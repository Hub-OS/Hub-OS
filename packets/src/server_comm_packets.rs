// Increment VERSION_ITERATION src/packets/mod.rs if packets are added or modified

use serde::{Deserialize, Serialize};
use strum::IntoStaticStr;

#[derive(Debug, Serialize, Deserialize, IntoStaticStr)]
pub enum ServerCommPacket {
    Poll,
    Alive,
    Message { data: Vec<u8> },
}
