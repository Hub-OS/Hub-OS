use rand::seq::IndexedRandom;

use super::BattleInitMusic;
use crate::resources::{Globals, FIELD_DEFAULT_WIDTH};
use std::collections::HashSet;

const DEFAULT_PLAYER_LAYOUTS: [(i32, i32); 9] = [
    (2, 2), // center
    (1, 3), // bottom left
    (1, 1), // top left
    (3, 3), // bottom right
    (3, 1), // top right
    (1, 2), // back
    (3, 2), // front
    (2, 1), // top
    (2, 3), // bottom
];

pub struct BattleConfig {
    pub player_spawn_positions: Vec<(i32, i32)>,
    pub spectators: HashSet<usize>,
    pub spectate_on_delete: bool,
    pub player_flippable: Vec<Option<bool>>,
    pub turn_limit: Option<u32>,
    pub automatic_turn_end: bool,
    pub automatic_battle_end: bool,
    pub automatic_scene_end: bool,
    // todo:
    // pub status_durations: [FrameTime; 3],
    // pub intangibility_duration: FrameTime,
    // pub super_effective_multiplier: f32,
    pub battle_init_music: Option<BattleInitMusic>,
}

impl BattleConfig {
    pub fn new(globals: &Globals, player_count: usize) -> Self {
        let mut player_spawn_positions = Vec::with_capacity(player_count);

        for i in 0..player_count {
            let layout_index = (i / 2) % DEFAULT_PLAYER_LAYOUTS.len();
            let mut position = DEFAULT_PLAYER_LAYOUTS[layout_index];

            if i % 2 == 1 {
                position.0 = FIELD_DEFAULT_WIDTH as i32 - position.0 - 1;
            }

            player_spawn_positions.push(position);
        }

        Self {
            spectators: Default::default(),
            spectate_on_delete: false,
            player_spawn_positions,
            player_flippable: vec![None; player_count],
            turn_limit: None,
            automatic_turn_end: false,
            automatic_battle_end: true,
            automatic_scene_end: true,
            // status_durations: [90, 120, 150],
            // intangibility_duration: 120,
            // super_effective_multiplier: 2.0,
            battle_init_music: Some(BattleInitMusic {
                buffer: globals
                    .music
                    .battle
                    .choose(&mut rand::rng())
                    .cloned()
                    .unwrap_or_default(),
                loops: true,
            }),
        }
    }
}
