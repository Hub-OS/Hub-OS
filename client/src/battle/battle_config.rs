use super::BattleInitMusic;
use crate::resources::Globals;

const DEFAULT_PLAYER_LAYOUTS: [[(i32, i32); 4]; 4] = [
    [(2, 2), (0, 0), (0, 0), (0, 0)],
    [(2, 2), (5, 2), (0, 0), (0, 0)],
    [(2, 2), (4, 3), (6, 1), (0, 0)],
    [(1, 3), (3, 1), (4, 3), (6, 1)],
];

#[derive(Clone)]
pub struct BattleConfig {
    pub player_spawn_positions: Vec<(i32, i32)>,
    pub player_flippable: Vec<Option<bool>>,
    pub turn_limit: Option<u32>,
    pub automatic_turn_end: bool,
    // todo:
    // pub status_durations: [FrameTime; 3],
    // pub intangibility_duration: FrameTime,
    // pub super_effective_multiplier: f32,
    pub battle_init_music: Option<BattleInitMusic>,
}

impl BattleConfig {
    pub fn new(globals: &Globals, player_count: usize) -> Self {
        let spawn_count = player_count.min(4);
        let mut player_spawn_positions = DEFAULT_PLAYER_LAYOUTS[spawn_count - 1].to_vec();
        player_spawn_positions.resize(spawn_count, (0, 0));

        Self {
            player_spawn_positions,
            player_flippable: vec![None; spawn_count],
            turn_limit: None,
            automatic_turn_end: false,
            // status_durations: [90, 120, 150],
            // intangibility_duration: 120,
            // super_effective_multiplier: 2.0,
            // turn_limit: None,
            battle_init_music: Some(BattleInitMusic {
                buffer: globals.music.battle.clone(),
                loops: true,
            }),
        }
    }
}
