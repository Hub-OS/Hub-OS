use crate::structures::{FileHash, PackageCategory};
use serde::{Deserialize, Serialize};
use strum::IntoStaticStr;

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
        player_package: String,
        // package_id, code
        cards: Vec<(String, String)>,
        // todo: blocks
    },
    PackageList {
        index: usize,
        // category, package_id, hash
        packages: Vec<(PackageCategory, String, FileHash)>,
    },
    MissingPackages {
        index: usize,
        recipient_index: usize,
        list: Vec<FileHash>,
    },
    PackageZip {
        index: usize,
        data: Vec<u8>,
    },
    Ready {
        index: usize,
        seed: u64,
    },
    Input {
        index: usize,
        pressed: Vec<u8>,
        buffer_sizes: Vec<usize>,
    },
    // Disconnect { index: usize }, // todo: use in netplay scene
    AllDisconnected,
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
            NetplayPacket::PackageZip { index, .. } => *index,
            NetplayPacket::Ready { index, .. } => *index,
            NetplayPacket::Input { index, .. } => *index,
            NetplayPacket::AllDisconnected => 0,
        }
    }
}
