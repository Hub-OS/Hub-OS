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
}

impl BattleConfig {
    pub fn new(player_count: usize) -> Self {
        let spawn_count = player_count.min(4);
        let mut player_spawn_positions = DEFAULT_PLAYER_LAYOUTS[spawn_count - 1].to_vec();
        player_spawn_positions.resize(spawn_count, (0, 0));

        Self {
            player_spawn_positions,
            player_flippable: vec![None; spawn_count],
            turn_limit: None,
            automatic_turn_end: false,
        }
    }
}
