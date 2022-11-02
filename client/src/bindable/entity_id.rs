#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct EntityID(hecs::Entity);

impl EntityID {
    pub const DANGLING: EntityID = EntityID(hecs::Entity::DANGLING);
}

impl Default for EntityID {
    fn default() -> Self {
        Self(hecs::Entity::DANGLING)
    }
}

impl From<hecs::Entity> for EntityID {
    fn from(entity: hecs::Entity) -> Self {
        EntityID(entity)
    }
}

impl From<EntityID> for hecs::Entity {
    fn from(id: EntityID) -> Self {
        id.0
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for EntityID {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let number = match lua_value {
            rollback_mlua::Value::Integer(number) => number,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "EntityID",
                    message: None,
                })
            }
        };

        let id = match hecs::Entity::from_bits(number as u64) {
            Some(id) => id,
            None => {
                return Err(rollback_mlua::Error::RuntimeError(String::from(
                    "invalid entity id",
                )))
            }
        };

        Ok(EntityID(id))
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for EntityID {
    fn to_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        Ok(rollback_mlua::Value::Integer(self.0.to_bits().get() as i64))
    }
}
