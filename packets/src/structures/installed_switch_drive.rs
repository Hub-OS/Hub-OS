use super::{PackageId, SwitchDriveSlot};
use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct InstalledSwitchDrive {
    pub package_id: PackageId,
    pub slot: SwitchDriveSlot,
}
