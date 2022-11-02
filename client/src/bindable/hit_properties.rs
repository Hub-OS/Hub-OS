use crate::bindable::*;

#[derive(Clone, Copy)]
pub struct HitProperties {
    pub damage: i32,
    pub flags: HitFlags,
    pub element: Element,
    pub secondary_element: Element,
    pub aggressor: EntityID,
    pub drag: Drag, // only used if HitFlags::Drag is set
    pub context: HitContext,
}

impl Default for HitProperties {
    fn default() -> Self {
        HitProperties {
            damage: 0,
            flags: HitFlag::FLINCH | HitFlag::IMPACT,
            element: Element::None,
            secondary_element: Element::None,
            aggressor: EntityID::DANGLING,
            drag: Drag::default(),
            context: HitContext::default(),
        }
    }
}

impl HitProperties {
    pub fn blank() -> Self {
        HitProperties {
            damage: 0,
            flags: 0,
            element: Element::None,
            secondary_element: Element::None,
            aggressor: EntityID::DANGLING,
            drag: Drag::default(),
            context: HitContext::default(),
        }
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
            damage: table.get("damage").unwrap_or_default(),
            flags: table.get("flags").unwrap_or_default(),
            element: table.get("element").unwrap_or_default(),
            secondary_element: table.get("secondary_element").unwrap_or_default(),
            aggressor: table.get("aggressor").unwrap_or_default(),
            drag: table.get("drag").unwrap_or_default(),
            context: table.get("context").unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for HitProperties {
    fn to_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        (&self).to_lua(lua)
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for &HitProperties {
    fn to_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;
        table.set("damage", self.damage)?;
        table.set("flags", self.flags)?;
        table.set("element", self.element)?;
        table.set("secondary_element", self.secondary_element)?;
        table.set("aggressor", self.aggressor)?;
        table.set("drag", self.drag)?;
        table.set("context", &self.context)?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
