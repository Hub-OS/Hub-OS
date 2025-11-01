use crate::render::{CameraMotion, FrameTime};
use framework::prelude::{Color, UVec2, Vec2};

pub const DEFAULT_PACKAGE_REPO: &str = "https://hubos.dev";

// 1 MiB
pub const BATTLE_VM_MEMORY: usize = 1024 * 1024;
pub const INPUT_BUFFER_LIMIT: usize = 20;
pub const IDENTITY_LEN: usize = 32;

pub const RESOLUTION: UVec2 = UVec2::new(240, 160);
pub const RESOLUTION_F: Vec2 = Vec2::new(RESOLUTION.x as _, RESOLUTION.y as _);
pub const MAX_VOLUME: u8 = 100;

// battle
pub const DEFAULT_INPUT_DELAY: u8 = 2;
pub const MAX_INPUT_DELAY: u8 = 15;
pub const BATTLE_UI_MARGIN: f32 = 2.0;
pub const CARD_SELECT_CARD_COLS: usize = 5;
pub const CARD_SELECT_COLS: usize = CARD_SELECT_CARD_COLS + 1;
pub const CARD_SELECT_ROWS: usize = 2;
pub const LOW_HP_SFX_RATE: FrameTime = 45;
pub const BATTLE_CAMERA_OFFSET: Vec2 = Vec2::new(0.0, 8.0);
pub const BATTLE_CARD_SELECT_CAMERA_OFFSET: Vec2 = Vec2::new(0.0, -7.0);
pub const BATTLE_ZOOM_MOTION: CameraMotion = CameraMotion::Multiply { factor: 0.08 };
pub const BATTLE_PAN_MOTION: CameraMotion = CameraMotion::Multiply { factor: 0.08 };
pub const BATTLE_CAMERA_TILE_PADDING: f32 = 1.0;
pub const FIELD_DEFAULT_WIDTH: usize = 8;
pub const FIELD_DEFAULT_HEIGHT: usize = 5;

// tile states
pub const BROKEN_LIFETIME: FrameTime = 600;
pub const TILE_FLICKER_DURATION: FrameTime = 60;
pub const TEMP_TEAM_DURATION: FrameTime = 1800;

// statuses
pub const DRAG_PER_TILE_DURATION: FrameTime = 4;
pub const DRAG_LOCKOUT: u8 = 22;

// overworld
pub const OVERWORLD_WALK_SPEED: f32 = 1.0;
pub const OVERWORLD_RUN_SPEED: f32 = 2.0;
pub const OVERWORLD_RUN_THRESHOLD: f32 = 1.5;

// text colors
pub const TEXTBOX_TEXT_COLOR: Color = Color::new(0.2, 0.2, 0.3, 1.0);
pub const TEXT_TRANSPARENT_SHADOW_COLOR: Color = Color::new(0.4, 0.4, 0.4, 0.3);
pub const TEXT_DARK_SHADOW_COLOR: Color = Color::new(0.32, 0.388, 0.45, 1.0);
pub const CONTEXT_TEXT_SHADOW_COLOR: Color = Color::new(0.06, 0.31, 0.41, 1.0);

// sizes
pub const CARD_PREVIEW_SIZE: Vec2 = Vec2::new(56.0, 48.0);
