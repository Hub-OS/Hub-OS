use super::{Actor, Direction, PlayerData, WidgetTracker};
use std::collections::HashSet;
use std::collections::VecDeque;
use std::net::SocketAddr;

#[derive(Default)]
pub(super) struct BattleTrackingInfo {
    pub plugin_index: usize,
    pub player_index: usize,
    pub remote_addresses: Vec<SocketAddr>,
}

pub(super) struct Client {
    pub socket_address: SocketAddr,
    pub actor: Actor,
    pub warp_in: bool,
    pub warp_area: String,
    pub warp_x: f32,
    pub warp_y: f32,
    pub warp_z: f32,
    pub warp_direction: Direction,
    pub ready: bool,
    pub transferring: bool,
    pub area_join_time: u64,
    pub cached_assets: HashSet<String>,
    pub texture_buffer: Vec<u8>,
    pub animation_buffer: Vec<u8>,
    pub mugshot_texture_buffer: Vec<u8>,
    pub mugshot_animation_buffer: Vec<u8>,
    pub widget_tracker: WidgetTracker<usize>,
    pub battle_tracker: VecDeque<BattleTrackingInfo>,
    pub player_data: PlayerData,
    pub is_input_locked: bool,
}

impl Client {
    #[allow(clippy::too_many_arguments)]
    pub(super) fn new(
        socket_address: SocketAddr,
        name: String,
        identity: Vec<u8>,
        area_id: String,
        spawn_x: f32,
        spawn_y: f32,
        spawn_z: f32,
        spawn_direction: Direction,
    ) -> Client {
        use super::asset;
        use std::time::Instant;
        use uuid::Uuid;

        let id = Uuid::new_v4().to_string();

        Client {
            socket_address,
            actor: Actor {
                id: id.clone(),
                name,
                area_id,
                texture_path: asset::get_player_texture_path(&id),
                animation_path: asset::get_player_animation_path(&id),
                mugshot_texture_path: asset::get_player_mugshot_texture_path(&id),
                mugshot_animation_path: asset::get_player_mugshot_animation_path(&id),
                direction: spawn_direction,
                x: spawn_x,
                y: spawn_y,
                z: spawn_z,
                last_movement_time: Instant::now(),
                scale_x: 1.0,
                scale_y: 1.0,
                rotation: 0.0,
                map_color: (248, 248, 0, 255),
                current_animation: None,
                solid: false,
            },
            warp_in: true,
            warp_area: String::new(),
            warp_x: spawn_x,
            warp_y: spawn_y,
            warp_z: spawn_z,
            warp_direction: spawn_direction,
            ready: false,
            transferring: false,
            area_join_time: 0,
            cached_assets: HashSet::new(),
            texture_buffer: Vec::new(),
            animation_buffer: Vec::new(),
            mugshot_texture_buffer: Vec::new(),
            mugshot_animation_buffer: Vec::new(),
            widget_tracker: WidgetTracker::new(),
            battle_tracker: VecDeque::new(),
            player_data: PlayerData::new(identity),
            is_input_locked: false,
        }
    }

    pub fn is_in_widget(&self) -> bool {
        !self.widget_tracker.is_empty()
    }

    pub fn is_shopping(&self) -> bool {
        self.widget_tracker.current_shop().is_some()
    }

    pub fn is_battling(&self) -> bool {
        !self.battle_tracker.is_empty()
    }

    pub fn is_busy(&self) -> bool {
        self.is_battling() || self.is_in_widget()
    }
}

pub(super) fn find_longest_frame_length(animation_data: &str) -> u32 {
    animation_data
        .lines()
        .map(|line| line.trim())
        .filter(|line| line.starts_with("frame"))
        .fold(0, |longest_length, line| {
            let width: i32 = value_of(line, "w").unwrap_or_default();
            let width = width.wrapping_abs() as u32;

            if width > longest_length {
                return width;
            }

            let height: i32 = value_of(line, "h").unwrap_or_default();
            let height = height.wrapping_abs() as u32;

            if height > longest_length {
                return height;
            }

            longest_length
        })
}

fn value_of<T>(line: &str, key: &str) -> Option<T>
where
    T: std::str::FromStr,
{
    let key_index = line.find(key)?;

    // based on ValueOf in bnAnimation.cpp
    // skips the = and ", but technically could be any two values here
    let value_start_index = key_index + key.len() + 2;

    if value_start_index >= line.len() {
        return None;
    }

    let value_slice = &line[value_start_index..];

    let value_end_index = value_slice.find('"')?;
    let value_slice = &value_slice[..value_end_index];

    value_slice.parse().ok()
}
