use crate::structures::{FileHash, Input, InstalledBlock, PackageCategory, PackageId};
use serde::{Deserialize, Serialize};
use strum::IntoStaticStr;

#[derive(Clone, PartialEq, Eq, Debug, Serialize, Deserialize)]
pub enum NetplaySignal {
    AttemptingFlee,
    CompletedFlee,
    Disconnect,
}

#[derive(Clone, PartialEq, Eq, Debug, Serialize, Deserialize)]
pub struct NetplayBufferItem {
    pub pressed: Vec<Input>,
    pub signals: Vec<NetplaySignal>,
}

#[derive(Clone, PartialEq, Eq, Debug, Serialize, Deserialize, IntoStaticStr)]
pub enum NetplayPacket {
    Heartbeat {
        index: usize,
    },
    Hello {
        index: usize,
    },
    HelloAck {
        index: usize,
    },
    PlayerSetup {
        index: usize,
        player_package: PackageId,
        // package_id, code
        cards: Vec<(PackageId, String)>,
        blocks: Vec<InstalledBlock>,
    },
    PackageList {
        index: usize,
        // category, package_id, hash
        packages: Vec<(PackageCategory, PackageId, FileHash)>,
    },
    MissingPackages {
        index: usize,
        recipient_index: usize,
        list: Vec<FileHash>,
    },
    ReadyForPackages {
        index: usize,
    },
    PackageZip {
        index: usize,
        data: Vec<u8>,
    },
    Ready {
        index: usize,
        seed: u64,
    },
    Buffer {
        index: usize,
        data: NetplayBufferItem,
        buffer_sizes: Vec<usize>,
    },
}

impl NetplayPacket {
    pub fn index(&self) -> usize {
        match self {
            NetplayPacket::Hello { index } => *index,
            NetplayPacket::HelloAck { index } => *index,
            NetplayPacket::Heartbeat { index } => *index,
            NetplayPacket::PlayerSetup { index, .. } => *index,
            NetplayPacket::PackageList { index, .. } => *index,
            NetplayPacket::MissingPackages { index, .. } => *index,
            NetplayPacket::ReadyForPackages { index } => *index,
            NetplayPacket::PackageZip { index, .. } => *index,
            NetplayPacket::Ready { index, .. } => *index,
            NetplayPacket::Buffer { index, .. } => *index,
        }
    }
}
