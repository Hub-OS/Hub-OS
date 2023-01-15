use crate::bindable::BlockColor;
use serde::{Deserialize, Serialize};

#[derive(Default, Clone, Serialize, Deserialize)]
pub struct InstalledBlock {
    pub package_id: String,
    pub rotation: u8,
    pub color: BlockColor,
    pub position: (usize, usize),
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
