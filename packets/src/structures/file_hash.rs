use serde::{Deserialize, Serialize};
use std::fmt::{Debug, Display};

#[derive(Default, Hash, PartialEq, Eq, Clone, Copy, Serialize, Deserialize)]
pub struct FileHash {
    bytes: [u8; 32],
}

impl FileHash {
    pub const ZERO: FileHash = FileHash { bytes: [0; 32] };

    pub fn hash(data: &[u8]) -> Self {
        let mut hasher = hmac_sha256::Hash::new();
        hasher.update(data);

        Self {
            bytes: hasher.finalize(),
        }
    }

    pub fn from_prehashed(bytes: [u8; 32]) -> Self {
        Self { bytes }
    }

    pub fn from_hex(value: &str) -> Option<Self> {
        let bytes = hex::decode(value).ok()?;
        let bytes = bytes.try_into().ok()?;

        Some(Self { bytes })
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.bytes
    }
}

impl Display for FileHash {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for byte in self.bytes {
            write!(f, "{byte:02x}")?;
        }

        Ok(())
    }
}

impl Debug for FileHash {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        Display::fmt(self, f)
    }
}
