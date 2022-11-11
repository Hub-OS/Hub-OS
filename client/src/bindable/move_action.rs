use super::Direction;
use crate::battle::BattleCallback;
use crate::render::FrameTime;

#[derive(Default, Clone)]
pub struct MoveAction {
    pub success: bool,
    pub progress: FrameTime,
    pub delta_frames: FrameTime, // Frames between tile A and B. If 0, teleport. Else, we could be sliding
    pub delay_frames: FrameTime, // Startup lag to be used with animations
    pub endlag_frames: FrameTime, // Wait period before action is complete
    pub height: f32, // If this is non-zero with delta frames, the character will effectively jump
    pub dest: (i32, i32),
    pub source: (i32, i32),
    pub on_begin: Option<BattleCallback>,
}

impl MoveAction {
    pub fn teleport(dest: (i32, i32)) -> Self {
        Self {
            dest,
            ..Default::default()
        }
    }

    pub fn slide(dest: (i32, i32), duration: FrameTime) -> Self {
        Self {
            dest,
            delta_frames: duration,
            ..Default::default()
        }
    }

    pub fn jump(dest: (i32, i32), height: f32, duration: FrameTime) -> Self {
        Self {
            dest,
            height,
            delta_frames: duration,
            ..Default::default()
        }
    }

    pub fn is_jumping(&self) -> bool {
        self.height > 0.0 && self.delta_frames > 0
    }

    pub fn is_sliding(&self) -> bool {
        self.delta_frames > 0 && self.height <= 0.0
    }

    pub fn is_teleporting(&self) -> bool {
        self.delta_frames == 0 && self.height == 0.0
    }

    pub fn is_in_endlag(&self) -> bool {
        self.progress >= self.delay_frames + self.delta_frames
    }

    pub fn animation_progress_percent(&self) -> f32 {
        if self.progress < self.delay_frames {
            return 0.0;
        }

        let percent = (self.progress - self.delay_frames) as f32 / self.delta_frames as f32;

        percent.min(1.0)
    }

    pub fn is_complete(&self) -> bool {
        self.progress >= self.delay_frames + self.delta_frames + self.endlag_frames
    }

    pub fn direction(&self) -> Direction {
        let x_diff = self.dest.0 - self.source.0;
        let y_diff = self.dest.1 - self.source.1;

        let mut direction = Direction::None;

        if x_diff < 0 {
            direction = Direction::Left;
        } else if x_diff > 0 {
            direction = Direction::Right;
        }

        if y_diff < 0 {
            direction = direction.join(Direction::Up);
        } else if y_diff > 0 {
            direction = direction.join(Direction::Down);
        }

        direction
    }

    pub fn aligned_direction(&self) -> Direction {
        let (horizontal, vertical) = self.direction().split();

        if horizontal == Direction::None {
            // fallback to vertical
            return vertical;
        }

        // prefer horizontal
        horizontal
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for MoveAction {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "MoveAction",
                    message: None,
                })
            }
        };

        let dest_table: rollback_mlua::Table = table.get("dest")?;
        let dest = (dest_table.raw_get("#x")?, dest_table.raw_get("#y")?);

        let on_begin: Option<rollback_mlua::Function> = table.get("on_begin")?;
        let vm_index = lua.named_registry_value("vm_index")?;
        let on_begin = on_begin
            .map(|func| BattleCallback::new_lua_callback(lua, vm_index, func))
            .transpose()?;

        Ok(Self {
            success: false,
            progress: 0,
            delta_frames: table.get("delta_frames").unwrap_or_default(),
            delay_frames: table.get("delay_frames").unwrap_or_default(),
            endlag_frames: table.get("endlag_frames").unwrap_or_default(),
            height: table.get("height").unwrap_or_default(),
            dest,
            source: (0, 0),
            on_begin,
        })
    }
}
