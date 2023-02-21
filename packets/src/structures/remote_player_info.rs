use serde::{Deserialize, Serialize};
use std::net::SocketAddr;

#[derive(PartialEq, Eq, Clone, Debug, Serialize, Deserialize)]
pub struct RemotePlayerInfo {
    pub address: SocketAddr,
    pub index: usize,
    pub health: i32,
    pub base_health: i32,
}
