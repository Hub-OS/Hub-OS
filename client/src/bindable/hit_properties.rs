use crate::battle::AttackContext;
use crate::bindable::*;
use crate::render::FrameTime;
use crate::structures::VecMap;

#[derive(Clone)]
pub struct HitProperties {
    pub damage: i32,
    pub flags: HitFlags,
    pub durations: VecMap<HitFlags, FrameTime>,
    pub element: Element,
    pub secondary_element: Element,
    pub drag: Drag, // only used if HitFlags::Drag is set
    pub context: AttackContext,
}

impl Default for HitProperties {
    fn default() -> Self {
        HitProperties {
            damage: 0,
            flags: HitFlag::FLINCH | HitFlag::IMPACT,
            durations: Default::default(),
            element: Element::None,
            secondary_element: Element::None,
            drag: Drag::default(),
            context: AttackContext::default(),
        }
    }
}

impl HitProperties {
    pub fn blank() -> Self {
        HitProperties {
            damage: 0,
            flags: 0,
            durations: Default::default(),
            element: Element::None,
            secondary_element: Element::None,
            drag: Drag::default(),
            context: AttackContext::default(),
        }
    }

    pub fn drags(&self) -> bool {
        self.drag.distance > 0
            && self.drag.direction != Direction::None
            && self.flags & HitFlag::DRAG != HitFlag::NONE
    }

    pub fn is_super_effective(&self, element: Element) -> bool {
        element.is_weak_to(self.element) || element.is_weak_to(self.secondary_element)
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for HitProperties {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "HitProperties",
                    message: None,
                })
            }
        };

        Ok(HitProperties {
            damage: table.raw_get("damage").unwrap_or_default(),
            flags: table.get("flags").unwrap_or_default(),
            durations: VecMap::from_lua_table(table.get("status_durations")?),
            element: table.raw_get("element").unwrap_or_default(),
            secondary_element: table.raw_get("secondary_element").unwrap_or_default(),
            drag: table.raw_get("drag").unwrap_or_default(),
            context: table.raw_get("context").unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for HitProperties {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        (&self).into_lua(lua)
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for &HitProperties {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;
        table.raw_set("damage", self.damage)?;
        table.raw_set("flags", self.flags)?;
        table.raw_set("status_durations", self.durations.to_lua_table(lua)?)?;
        table.raw_set("element", self.element)?;
        table.raw_set("secondary_element", self.secondary_element)?;
        table.raw_set("drag", self.drag)?;
        table.raw_set("context", &self.context)?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
