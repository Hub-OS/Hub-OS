use crate::{
    bindable::{EntityId, Team},
    lua_api::create_entity_table,
};

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum EntityOwner {
    Team(Team),
    Entity(Team, EntityId),
}

impl<'lua> rollback_mlua::IntoLua<'lua> for EntityOwner {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        match self {
            EntityOwner::Team(team) => team.into_lua(lua),
            EntityOwner::Entity(_, id) => create_entity_table(lua, id)?.into_lua(lua),
        }
    }
}
