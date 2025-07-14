use crate::structures::FileHash;
use serde::{Deserialize, Serialize};

pub trait AssetTrait {
    fn hash(&self) -> FileHash;
    fn last_modified(&self) -> u64;
    fn cache_to_disk(&self) -> bool;
    fn data(&self) -> &AssetData;
}

#[derive(Serialize, Deserialize, PartialEq, Eq, Clone, Debug)]
pub enum AssetDataType {
    Text,
    CompressedText,
    Texture,
    Audio,
    Data,
    Unknown,
}

impl AssetDataType {
    pub fn from_path_str(path: &str) -> Self {
        let (_, extension) = path.rsplit_once('.').unwrap_or_default();

        match extension.to_lowercase().as_str() {
            "png" | "bmp" => Self::Texture,
            "flac" | "mp3" | "wav" | "mid" | "midi" | "ogg" => Self::Audio,
            "zip" => Self::Data,
            "toml" | "lua" | "animation" => Self::Text,
            _ => Self::Unknown,
        }
    }
}

#[derive(Clone, Debug)]
pub enum AssetData {
    Text(String),
    CompressedText(Vec<u8>),
    Texture(Vec<u8>),
    Audio(Vec<u8>),
    Data(Vec<u8>),
}

impl AssetData {
    pub fn compress_text(text: String) -> AssetData {
        use flate2::write::ZlibEncoder;
        use flate2::Compression;
        use std::io::prelude::*;

        let mut e = ZlibEncoder::new(Vec::new(), Compression::fast());

        if e.write_all(text.as_bytes()).is_err() {
            return AssetData::Text(text);
        }

        if let Ok(bytes) = e.finish() {
            return AssetData::CompressedText(bytes);
        }

        AssetData::Text(text)
    }

    pub fn data_type(&self) -> AssetDataType {
        match self {
            Self::Text(_) => AssetDataType::Text,
            Self::CompressedText(_) => AssetDataType::CompressedText,
            Self::Texture(_) => AssetDataType::Texture,
            Self::Audio(_) => AssetDataType::Audio,
            Self::Data(_) => AssetDataType::Data,
        }
    }
}
