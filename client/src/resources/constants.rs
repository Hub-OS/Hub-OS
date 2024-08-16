use crate::render::FrameTime;
use framework::prelude::{Color, UVec2, Vec2};

pub const DEFAULT_PACKAGE_REPO: &str = "https://hubos.dev";

// 1 MiB
pub const BATTLE_VM_MEMORY: usize = 1024 * 1024;
pub const INPUT_BUFFER_LIMIT: usize = 20;
pub const IDENTITY_LEN: usize = 32;

pub const RESOLUTION_F: Vec2 = Vec2::new(240.0, 160.0);
pub const DEFAULT_SCALE: f32 = 2.0;
pub const TRUE_RESOLUTION: UVec2 = UVec2::new(
    (RESOLUTION_F.x * DEFAULT_SCALE) as u32,
    (RESOLUTION_F.y * DEFAULT_SCALE) as u32,
);
pub const MAX_VOLUME: u8 = 100;

// battle
pub const INPUT_DELAY: usize = 2;
pub const BATTLE_UI_MARGIN: f32 = 2.0;
pub const CARD_SELECT_CARD_COLS: usize = 5;
pub const CARD_SELECT_COLS: usize = CARD_SELECT_CARD_COLS + 1;
pub const CARD_SELECT_ROWS: usize = 2;
pub const LOW_HP_SFX_RATE: FrameTime = 45;
pub const BATTLE_CAMERA_OFFSET: Vec2 = Vec2::new(0.0, 8.0);
pub const BATTLE_CARD_SELECT_CAMERA_OFFSET: Vec2 = Vec2::new(0.0, -7.0);

// tile states
pub const POISON_INTERVAL: FrameTime = 7;
pub const BROKEN_LIFETIME: FrameTime = 600;
pub const TILE_FLICKER_DURATION: FrameTime = 60;
pub const TEMP_TEAM_DURATION: FrameTime = 1800;

// statuses
pub const DEFAULT_STATUS_DURATION: FrameTime = 90;
pub const DEFAULT_INTANGIBILITY_DURATION: FrameTime = 120;
pub const DRAG_PER_TILE_DURATION: FrameTime = 4;
pub const DRAG_LOCKOUT: FrameTime = 22;

// overworld
pub const OVERWORLD_WALK_SPEED: f32 = 1.0;
pub const OVERWORLD_RUN_SPEED: f32 = 2.0;
pub const OVERWORLD_RUN_THRESHOLD: f32 = 1.5;

// text colors
pub const TEXT_TRANSPARENT_SHADOW_COLOR: Color = Color::new(0.4, 0.4, 0.4, 0.3);
pub const TEXT_DARK_SHADOW_COLOR: Color = Color::new(0.32, 0.388, 0.45, 1.0);
pub const CONTEXT_TEXT_SHADOW_COLOR: Color = Color::new(0.06, 0.31, 0.41, 1.0);

// sizes
pub const CARD_PREVIEW_SIZE: Vec2 = Vec2::new(56.0, 48.0);
