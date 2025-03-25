// Increment VERSION_ITERATION lib.rs if packets are added or modified

use crate::structures::{
    FileHash, Input, InstalledBlock, InstalledSwitchDrive, PackageCategory, PackageId,
};
use serde::{Deserialize, Serialize};
use strum::IntoStaticStr;

#[derive(Clone, PartialEq, Eq, Debug, Serialize, Deserialize)]
pub enum NetplaySignal {
    AttemptingFlee,
    CompletedFlee,
    Disconnect,
}

#[derive(Default, Clone, PartialEq, Eq, Debug, Serialize, Deserialize)]
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
    Seed {
        index: usize,
        seed: u64,
    },
    PlayerSetup {
        index: usize,
        player_package: PackageId,
        script_enabled: bool,
        // package_id, code
        cards: Vec<(PackageId, String)>,
        regular_card: Option<usize>,
        recipes: Vec<PackageId>,
        blocks: Vec<InstalledBlock>,
        drives: Vec<InstalledSwitchDrive>,
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
    },
    Buffer {
        index: usize,
        data: NetplayBufferItem,
        lead: Vec<i16>,
    },
}

impl NetplayPacket {
    pub fn new_disconnect_signal(index: usize) -> NetplayPacket {
        NetplayPacket::Buffer {
            index,
            data: NetplayBufferItem {
                pressed: Vec::new(),
                signals: vec![NetplaySignal::Disconnect],
            },
            lead: Vec::new(),
        }
    }

    pub fn index(&self) -> usize {
        match self {
            NetplayPacket::Heartbeat { index } => *index,
            NetplayPacket::Hello { index } => *index,
            NetplayPacket::HelloAck { index } => *index,
            NetplayPacket::Seed { index, .. } => *index,
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
