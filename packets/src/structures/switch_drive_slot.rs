use enum_map::Enum;
use num_derive::FromPrimitive;
use serde::{Deserialize, Serialize};

#[repr(u8)]
#[derive(
    Enum, Default, Debug, Clone, Copy, Hash, PartialEq, Eq, FromPrimitive, Serialize, Deserialize,
)]
pub enum SwitchDriveSlot {
    #[default]
    Head,
    Body,
    Arms,
    Legs,
}

impl SwitchDriveSlot {
    pub fn state(&self) -> &'static str {
        match self {
            SwitchDriveSlot::Head => "HeadSlotIndicator",
            SwitchDriveSlot::Body => "BodySlotIndicator",
            SwitchDriveSlot::Arms => "ArmSlotIndicator",
            SwitchDriveSlot::Legs => "LegSlotIndicator",
        }
    }
}

impl From<String> for SwitchDriveSlot {
    fn from(text: String) -> Self {
        text.as_str().into()
    }
}

impl From<&str> for SwitchDriveSlot {
    fn from(text: &str) -> Self {
        match text.to_lowercase().as_str() {
            "head" => SwitchDriveSlot::Head,
            "body" => SwitchDriveSlot::Body,
            "arms" => SwitchDriveSlot::Arms,
            "legs" => SwitchDriveSlot::Legs,
            _ => SwitchDriveSlot::Body,
        }
    }
}
