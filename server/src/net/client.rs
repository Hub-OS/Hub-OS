use super::{Actor, Direction, PlayerData, WidgetTracker};
use packets::structures::{ActorId, BattleId};
use std::collections::HashSet;
use std::collections::VecDeque;
use std::net::SocketAddr;
use std::time::Instant;

pub(super) struct BattleTrackingInfo {
    pub battle_id: BattleId,
    pub creation_time: Instant,
    pub plugin_index: usize,
    pub player_index: usize,
    pub remote_addresses: Vec<SocketAddr>,
}

pub(super) struct Client {
    pub socket_address: SocketAddr,
    pub force_relay: bool,
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
    pub input_locks: usize,
}

impl Client {
    #[allow(clippy::too_many_arguments)]
    pub(super) fn new(
        socket_address: SocketAddr,
        id: ActorId,
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

        Client {
            socket_address,
            force_relay: true,
            actor: Actor {
                id,
                name,
                area_id,
                texture_path: asset::get_player_texture_path(id),
                animation_path: asset::get_player_animation_path(id),
                mugshot_texture_path: asset::get_player_mugshot_texture_path(id),
                mugshot_animation_path: asset::get_player_mugshot_animation_path(id),
                direction: if spawn_direction.is_none() {
                    Direction::Down
                } else {
                    spawn_direction
                },
                x: spawn_x,
                y: spawn_y,
                z: spawn_z,
                last_movement_time: Instant::now(),
                scale_x: 1.0,
                scale_y: 1.0,
                rotation: 0.0,
                map_color: (248, 248, 0, 255),
                current_animation: None,
                loop_animation: false,
                solid: false,
                child_sprites: Vec::new(),
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
            input_locks: 0,
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

            let height: i32 = value_of(line, "h").unwrap_or_default();
            let height = height.wrapping_abs() as u32;

            longest_length.max(width.max(height))
        })
}

fn value_of<T>(mut line: &str, key: &str) -> Option<T>
where
    T: std::str::FromStr,
{
    loop {
        line = line.trim_start();

        let Some(key_end) = line.find([' ', '=']) else {
            break;
        };

        let found_key = &line[..key_end];

        // strip key and whitespace
        line = line[key_end..].trim_start();

        if !line.starts_with('=') {
            continue;
        }

        // strip '=' and whitespace
        line = line[1..].trim_start();

        let value = if line.starts_with('"') {
            // strip quote
            line = &line[1..];

            // find the closing quote
            let Some(value_end) = line.find('"') else {
                // invalid line, exit early
                break;
            };

            let value = &line[..value_end];

            // strip value and end quote
            line = &line[value_end + 1..];

            value
        } else {
            // read until whitespace or the end of line
            let value_end = line.find(' ').unwrap_or(line.len());
            let value = &line[..value_end];

            // strip value
            line = &line[value_end..];

            value
        };

        if found_key == key {
            return value.parse().ok();
        }
    }

    None
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn animation_parsing() {
        const SAMPLE_A: &str = "animation\nframe x=100 w=\"80\" h=50\nframe x=100 w=\"80\" h=120";
        const SAMPLE_B: &str = "animation\nframe x=100 w = \"150\" h=50\nframe x=100 w=\"80\" h=90";

        assert_eq!(find_longest_frame_length(SAMPLE_A), 120);
        assert_eq!(find_longest_frame_length(SAMPLE_B), 150);

        // ensure no panics from invalid data
        find_longest_frame_length("");
        find_longest_frame_length(" ");
        find_longest_frame_length("fr");
        find_longest_frame_length("frame");
        find_longest_frame_length("frame w=");
        find_longest_frame_length("frame w=\"");
        find_longest_frame_length("frame w= h=30");
    }
}
