use super::Emotion;
use serde::{Deserialize, Serialize};

#[derive(Clone, Default, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub struct BattleStatistics {
    pub health: i32,
    pub won: bool,
    pub emotion: Emotion, // todo: track
    pub ran: bool,
    pub turns: u32,

    // used for score calculation
    pub boss_battle: bool,
    pub time: i64,
    pub hits_taken: usize,
    pub movements: usize,
    pub max_kill_chain: usize, // todo: track
    pub counters: usize,       // todo: track
}

impl BattleStatistics {
    pub fn new() -> Self {
        Self::default()
    }
}
