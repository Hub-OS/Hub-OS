use crate::bindable::{Drag, EntityId, HitFlag, HitFlags};
use crate::{render::FrameTime, structures::VecMap};

#[derive(Clone)]
pub struct AttackContext {
    pub aggressor: EntityId,
    pub flags: HitFlags,
    pub durations: VecMap<HitFlags, FrameTime>,
    pub drag: Drag,
}

impl AttackContext {
    pub fn new(entity_id: EntityId) -> Self {
        Self {
            aggressor: entity_id,
            flags: HitFlag::NONE,
            durations: Default::default(),
            drag: Drag::default(),
        }
    }
}

impl Default for AttackContext {
    fn default() -> Self {
        Self {
            aggressor: Default::default(),
            flags: HitFlag::NONE,
            durations: Default::default(),
            drag: Drag::default(),
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for AttackContext {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "AttackContext",
                    message: None,
                })
            }
        };

        Ok(AttackContext {
            aggressor: table.get("aggressor").unwrap_or_default(),
            flags: table.get("flags").unwrap_or_default(),
            durations: VecMap::from_lua_table(table.get("status_durations")?),
            drag: table.get("drag").unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for &AttackContext {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;
        table.set("aggressor", self.aggressor)?;
        table.set("flags", self.flags)?;
        table.set("status_durations", self.durations.to_lua_table(lua)?)?;

        if self.drag != Drag::default() {
            table.set("drag", self.drag)?;
        }

        Ok(rollback_mlua::Value::Table(table))
    }
}
