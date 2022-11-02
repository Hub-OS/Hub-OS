use super::Emotion;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct BattleSurvivor {
    pub id: String,
    pub health: u32,
}

#[derive(Clone, Default, Debug, Serialize, Deserialize)]
pub struct BattleStatistics {
    pub health: u32,      // todo: track
    pub emotion: Emotion, // todo: track
    pub ran: bool,        // todo: track
    pub turns: u32,
    pub score: i32,
    pub enemy_survivors: Vec<BattleSurvivor>, // todo: track
    pub neutral_survivors: Vec<BattleSurvivor>, // todo: track

    // used for score calculation
    pub boss_battle: bool,
    pub time: i64,
    pub hits_taken: usize, // todo: track
    pub movements: usize,
    pub max_kill_chain: usize, // todo: track
    pub counters: usize,       // todo: track
}

impl BattleStatistics {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn calculate_score(&mut self) {
        let mut score = 0;

        let duration_secs = self.time as f64 / 60.0;

        if self.boss_battle {
            if duration_secs > 50.01 {
                score += 4;
            } else if duration_secs > 40.01 {
                score += 6;
            } else if duration_secs > 30.01 {
                score += 8;
            } else {
                score += 10;
            }
        } else {
            #[allow(clippy::collapsible_else_if)]
            if duration_secs > 36.1 {
                score += 4;
            } else if duration_secs > 12.01 {
                score += 5;
            } else if duration_secs > 5.01 {
                score += 6;
            } else {
                score += 7;
            }
        }

        match self.hits_taken {
            0 => score += 1,
            1 => score += 0,
            2 => score -= 1,
            3 => score -= 2,
            _ => score -= 3,
        }

        if self.movements <= 2 {
            score += 1;
        }

        score += ((self.max_kill_chain.max(1) - 1) * 2) as i32;

        score += self.counters as i32;

        // score should be at least 1
        score = score.max(1);

        self.score = score;
    }
}
