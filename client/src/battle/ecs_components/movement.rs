use crate::battle::BattleCallback;
use crate::bindable::Direction;
use crate::lua_api::{create_movement_table, BEGIN_FN, END_FN, VM_INDEX_REGISTRY_KEY};
use crate::render::FrameTime;

#[derive(Default, Clone)]
pub struct Movement {
    pub success: bool,
    pub paused: bool,
    pub elapsed: FrameTime,
    pub delta: FrameTime, // Frames between tile A and B. If 0, teleport. Else, we could be sliding
    pub delay: FrameTime, // Startup lag to be used with animations
    pub endlag: FrameTime, // Wait period before action is complete
    pub height: f32, // If this is non-zero with delta frames, the character will effectively jump
    pub dest: (i32, i32),
    pub source: (i32, i32),
    pub begin_callback: Option<BattleCallback>,
    pub end_callback: Option<BattleCallback>,
}

impl Movement {
    pub fn teleport(dest: (i32, i32)) -> Self {
        Self {
            dest,
            ..Default::default()
        }
    }

    pub fn slide(dest: (i32, i32), duration: FrameTime) -> Self {
        Self {
            dest,
            delta: duration,
            ..Default::default()
        }
    }

    pub fn jump(dest: (i32, i32), height: f32, duration: FrameTime) -> Self {
        Self {
            dest,
            height,
            delta: duration,
            ..Default::default()
        }
    }

    pub fn is_jumping(&self) -> bool {
        self.height > 0.0 && self.delta > 0
    }

    pub fn is_sliding(&self) -> bool {
        self.delta > 0 && self.height <= 0.0
    }

    pub fn is_teleporting(&self) -> bool {
        self.delta == 0 && self.height == 0.0
    }

    pub fn is_in_endlag(&self) -> bool {
        self.elapsed >= self.delay + self.delta
    }

    pub fn animation_progress_percent(&self) -> f32 {
        if self.elapsed < self.delay {
            return 0.0;
        }

        let percent = (self.elapsed - self.delay) as f32 / self.delta as f32;

        percent.min(1.0)
    }

    pub fn is_complete(&self) -> bool {
        self.elapsed >= self.delay + self.delta + self.endlag
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

    pub fn interpolate_jump_height(&self, progress: f32) -> f32 {
        self.height * crate::ease::quadratic(progress)
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for Movement {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "Movement",
                    message: None,
                })
            }
        };

        let dest_table: rollback_mlua::Table = table.raw_get("dest_tile")?;
        let dest = (dest_table.raw_get("#x")?, dest_table.raw_get("#y")?);

        let on_begin: Option<rollback_mlua::Function> = table.raw_get(BEGIN_FN)?;
        let on_end: Option<rollback_mlua::Function> = table.raw_get(END_FN)?;

        let vm_index = lua.named_registry_value(VM_INDEX_REGISTRY_KEY)?;
        let on_begin = on_begin
            .map(|func| BattleCallback::new_lua_callback(lua, vm_index, func))
            .transpose()?;

        let on_end = on_end
            .map(|func| BattleCallback::new_lua_callback(lua, vm_index, func))
            .transpose()?;

        Ok(Self {
            success: false,
            paused: false,
            elapsed: 0,
            delta: table.raw_get("delta").unwrap_or_default(),
            delay: table.raw_get("delay").unwrap_or_default(),
            endlag: table.raw_get("endlag").unwrap_or_default(),
            height: table.raw_get("height").unwrap_or_default(),
            dest,
            source: (0, 0),
            begin_callback: on_begin,
            end_callback: on_end,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for Movement {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = create_movement_table(lua, &self)?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
