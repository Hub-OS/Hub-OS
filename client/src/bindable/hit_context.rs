use super::{EntityId, HitFlag, HitFlags};

#[derive(Clone, Copy)]
pub struct HitContext {
    pub aggressor: EntityId,
    pub flags: HitFlags,
}

impl Default for HitContext {
    fn default() -> Self {
        Self {
            aggressor: Default::default(),
            flags: HitFlag::NO_COUNTER,
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for HitContext {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "HitContext",
                    message: None,
                })
            }
        };

        Ok(HitContext {
            aggressor: table.get("aggressor").unwrap_or_default(),
            flags: table.get("flags").unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for &HitContext {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;
        table.set("aggressor", self.aggressor)?;
        table.set("flags", self.flags)?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
