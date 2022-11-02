use crate::structures::{FileHash, PackageCategory};
use serde::{Deserialize, Serialize};
use strum::IntoStaticStr;

#[derive(Clone, PartialEq, Eq, Debug, Serialize, Deserialize, IntoStaticStr)]
pub enum PvPPacket {
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
    // Disconnect { index: usize }, // todo: use in pvp scene
    AllDisconnected,
}

impl PvPPacket {
    pub fn index(&self) -> usize {
        match self {
            PvPPacket::Hello { index } => *index,
            PvPPacket::HelloAck { index } => *index,
            PvPPacket::Heartbeat { index } => *index,
            PvPPacket::PlayerSetup { index, .. } => *index,
            PvPPacket::PackageList { index, .. } => *index,
            PvPPacket::MissingPackages { index, .. } => *index,
            PvPPacket::PackageZip { index, .. } => *index,
            PvPPacket::Ready { index, .. } => *index,
            PvPPacket::Input { index, .. } => *index,
            PvPPacket::AllDisconnected => 0,
        }
    }
}
