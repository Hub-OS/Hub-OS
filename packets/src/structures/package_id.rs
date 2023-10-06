use serde::{Deserialize, Serialize};

#[derive(Default, Clone, PartialEq, Eq, Hash, Serialize, Deserialize, PartialOrd, Ord)]
pub struct PackageId(Box<str>);

impl PackageId {
    pub fn new_blank() -> Self {
        Self(Box::from(""))
    }

    pub fn is_blank(&self) -> bool {
        self.0.is_empty()
    }

    pub fn as_str(&self) -> &str {
        &self.0
    }
}

impl From<&str> for PackageId {
    fn from(id: &str) -> Self {
        Self(Box::from(id))
    }
}

impl From<String> for PackageId {
    fn from(id: String) -> Self {
        Self(Box::from(id))
    }
}

impl std::fmt::Display for PackageId {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        std::fmt::Display::fmt(&self.0, f)
    }
}

impl std::fmt::Debug for PackageId {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        std::fmt::Debug::fmt(&self.0, f)
    }
}

#[cfg(feature = "rollback_mlua")]
use rollback_mlua as mlua;

#[cfg(feature = "rollback_mlua")]
impl<'lua> mlua::FromLua<'lua> for PackageId {
    fn from_lua(lua_value: mlua::Value<'lua>, _: &'lua mlua::Lua) -> mlua::Result<Self> {
        let mlua::Value::String(id) = lua_value else {
            return Err(mlua::Error::FromLuaConversionError {
                from: lua_value.type_name(),
                to: "PackageId",
                message: None,
            });
        };

        Ok(Self(Box::from(id.to_string_lossy())))
    }
}

#[cfg(feature = "rollback_mlua")]
impl<'lua> mlua::IntoLua<'lua> for PackageId {
    fn into_lua(self, lua: &'lua mlua::Lua) -> mlua::Result<mlua::Value<'lua>> {
        lua.create_string(&*self.0).map(mlua::Value::String)
    }
}
