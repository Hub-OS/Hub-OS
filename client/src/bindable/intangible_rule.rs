use super::{Element, HitFlag, HitFlags};
use crate::battle::BattleCallback;
use crate::render::FrameTime;

#[derive(Clone)]
pub struct IntangibleRule {
    pub duration: FrameTime,
    pub hit_weaknesses: HitFlags,
    pub element_weaknesses: Vec<Element>,
    pub deactivate_callback: Option<BattleCallback>,
}

impl Default for IntangibleRule {
    fn default() -> Self {
        Self {
            duration: 120,
            hit_weaknesses: HitFlag::PIERCE_INVIS,
            element_weaknesses: Vec::new(),
            deactivate_callback: None,
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for IntangibleRule {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "IntangibleRule",
                    message: None,
                })
            }
        };

        Ok(IntangibleRule {
            duration: table.get("duration").unwrap_or_default(),
            hit_weaknesses: table.get("hit_weaknesses").unwrap_or_default(),
            element_weaknesses: table.get("element_weaknesses").unwrap_or_default(),
            deactivate_callback: table.get("on_deactivate_func").unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for IntangibleRule {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;
        table.set("duration", self.duration)?;
        table.set("hit_weaknesses", self.hit_weaknesses)?;
        table.set("element_weaknesses", &*self.element_weaknesses)?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
