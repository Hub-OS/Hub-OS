use crate::{
    bindable::{EntityId, Team},
    lua_api::create_entity_table,
};

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum EntityOwner {
    Team(Team),
    Entity(EntityId),
}

impl<'lua> rollback_mlua::FromLua<'lua> for EntityOwner {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        if let Ok(team) = Team::from_lua(lua_value.clone(), lua) {
            return Ok(Self::Team(team));
        }

        if let rollback_mlua::Value::Table(table) = lua_value {
            return Ok(Self::Entity(table.raw_get("#id")?));
        };

        Err(rollback_mlua::Error::FromLuaConversionError {
            from: lua_value.type_name(),
            to: "Entity or Team",
            message: None,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for EntityOwner {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        match self {
            EntityOwner::Team(team) => team.into_lua(lua),
            EntityOwner::Entity(id) => create_entity_table(lua, id)?.into_lua(lua),
        }
    }
}
