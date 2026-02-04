use super::{BlockColor, PackageId};
use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct InstalledBlock {
    pub package_id: PackageId,
    #[serde(default)]
    pub variant: usize,
    pub rotation: u8,
    pub color: BlockColor,
    pub position: (i8, i8),
}

impl InstalledBlock {
    /// Rotate clockwise
    pub fn rotate_c(&mut self) {
        self.rotation += 1;
        self.rotation %= 4;
    }

    /// Rotate counter clockwise
    pub fn rotate_cc(&mut self) {
        if self.rotation > 0 {
            self.rotation -= 1;
        } else {
            self.rotation = 3;
        }
    }
}
