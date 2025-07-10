use crate::structures::{PackageCategory, PackageId};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
pub enum SyncDataPacket {
    Heartbeat,
    RejectSync,
    AcceptSync,
    PackageList {
        list: Vec<(PackageCategory, PackageId)>,
    },
    RequestPackage {
        category: PackageCategory,
        id: PackageId,
    },
    Package {
        category: PackageCategory,
        id: PackageId,
        zip_bytes: Vec<u8>,
    },
    RequestSave,
    Save {
        save: Vec<u8>,
    },
    RequestIdentity,
    Identity {
        file_name: String,
        data: Vec<u8>,
        created_time: u64,
    },
    Complete {
        cancelled: bool,
    },
}
