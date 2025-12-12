// Increment VERSION_ITERATION lib.rs if packets are added or modified

use crate::structures::{
    FileHash, Input, InstalledBlock, InstalledSwitchDrive, MemoryCell, PackageCategory, PackageId,
};
use network_channels::Reliability;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use strum::IntoStaticStr;

#[derive(Clone, PartialEq, Eq, Debug, Serialize, Deserialize)]
pub enum NetplaySignal {
    AttemptingFlee,
    CompletedFlee,
    Disconnect,
    AcknowledgeServerMessage(usize),
}

#[derive(Default, Clone, PartialEq, Eq, Debug, Serialize, Deserialize)]
pub struct NetplayBufferItem {
    pub pressed: Vec<Input>,
    pub signals: Vec<NetplaySignal>,
}

#[derive(Clone, PartialEq, Serialize, Deserialize)]
pub struct NetplayPacket {
    pub index: usize,
    pub data: NetplayPacketData,
}

#[derive(Clone, PartialEq, Debug, Serialize, Deserialize, IntoStaticStr)]
pub enum NetplayPacketData {
    Heartbeat,
    Hello,
    HelloAck,
    Ping,
    Pong {
        sender: usize,
    },
    PlayerSetup {
        player_package: PackageId,
        script_enabled: bool,
        // package_id, code
        cards: Vec<(PackageId, String)>,
        regular_card: Option<usize>,
        recipes: Vec<PackageId>,
        blocks: Vec<InstalledBlock>,
        drives: Vec<InstalledSwitchDrive>,
        memories: HashMap<String, MemoryCell>,
        input_delay: usize,
    },
    PackageList {
        // category, package_id, hash
        packages: Vec<(PackageCategory, PackageId, FileHash)>,
    },
    MissingPackages {
        recipient_index: usize,
        list: Vec<FileHash>,
    },
    ReadyForPackages,
    PackageZip {
        data: Vec<u8>,
    },
    Ready,
    Buffer {
        data: NetplayBufferItem,
        frame_time: f32,
    },
}

impl NetplayPacket {
    pub fn default_reliability(&self) -> Reliability {
        if matches!(
            self.data,
            NetplayPacketData::Heartbeat | NetplayPacketData::Ping | NetplayPacketData::Pong { .. }
        ) {
            Reliability::Reliable
        } else {
            Reliability::ReliableOrdered
        }
    }

    pub fn prioritize(&self) -> bool {
        matches!(
            self.data,
            NetplayPacketData::Buffer { .. }
                | NetplayPacketData::Ping
                | NetplayPacketData::Pong { .. }
        )
    }

    pub fn new_disconnect_signal(index: usize) -> NetplayPacket {
        NetplayPacket {
            index,
            data: NetplayPacketData::Buffer {
                data: NetplayBufferItem {
                    pressed: Vec::new(),
                    signals: vec![NetplaySignal::Disconnect],
                },
                frame_time: 0.0,
            },
        }
    }
}
