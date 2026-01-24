use super::Emotion;
use serde::{Deserialize, Serialize};

#[derive(Clone, Default, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub struct BattleStatistics {
    pub health: i32,
    pub won: bool,
    pub emotion: Emotion, // todo: track
    pub ran: bool,
    pub turns: u32,
    pub time: i64,

    pub boss_battle: bool,
}

impl BattleStatistics {
    pub fn new() -> Self {
        Self::default()
    }
}
