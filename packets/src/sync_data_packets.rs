use crate::structures::{FileHash, PackageCategory, PackageId};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
pub enum SyncDataPacket {
    Heartbeat,
    RejectSync,
    AcceptSync,
    PassControl,
    RequestPackageList,
    PackageList {
        list: Vec<(PackageCategory, PackageId, FileHash)>,
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
    },
    Complete {
        cancelled: bool,
    },
}
