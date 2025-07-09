use crate::{VERSION_ID, VERSION_ITERATION};
use serde::{Deserialize, Serialize};
use std::sync::LazyLock;
use uuid::Uuid;

static LOCAL_ID: LazyLock<Uuid> = LazyLock::new(Uuid::new_v4);

#[derive(Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum MulticastPacket {
    Server { name: String },
    Client { name: String, uuid: Uuid },
    RequestSync { uuid: Uuid },
}

impl MulticastPacket {
    pub fn local_uuid() -> Uuid {
        *LOCAL_ID
    }
}

#[derive(Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct MulticastPacketWrapper<'a> {
    pub version_id: &'a str,
    pub version_iteration: u64,
    pub uuid: Uuid,
    pub data: MulticastPacket,
}

impl MulticastPacketWrapper<'_> {
    pub fn new(data: MulticastPacket) -> Self {
        Self {
            version_id: VERSION_ID,
            version_iteration: VERSION_ITERATION,
            uuid: *LOCAL_ID,
            data,
        }
    }

    pub fn is_compatible(&self) -> bool {
        self.version_iteration == VERSION_ITERATION
            && self.version_id == VERSION_ID
            && self.uuid != *LOCAL_ID
    }
}
