#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct EntityId(hecs::Entity);

impl EntityId {
    pub const DANGLING: EntityId = EntityId(hecs::Entity::DANGLING);
}

impl Default for EntityId {
    fn default() -> Self {
        Self(hecs::Entity::DANGLING)
    }
}

impl From<hecs::Entity> for EntityId {
    fn from(entity: hecs::Entity) -> Self {
        EntityId(entity)
    }
}

impl From<EntityId> for hecs::Entity {
    fn from(id: EntityId) -> Self {
        id.0
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for EntityId {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let number = match lua_value {
            rollback_mlua::Value::Integer(number) => number,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "EntityId",
                    message: None,
                })
            }
        };

        let Some(id) = hecs::Entity::from_bits(number as u64) else {
            return Err(rollback_mlua::Error::RuntimeError(String::from(
                "invalid entity id",
            )));
        };

        Ok(EntityId(id))
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for EntityId {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Integer(self.0.to_bits().get() as i64))
    }
}
