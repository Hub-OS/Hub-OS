use packets::structures::Uuid;
use serde::{Deserialize, Serialize};

#[derive(Clone, Serialize, Deserialize)]
pub struct ServerInfo {
    pub name: String,
    pub address: String,
    #[serde(default = "Uuid::new_v4")]
    pub uuid: Uuid,
    #[serde(default)]
    pub update_time: u64,
}
