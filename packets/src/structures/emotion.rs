use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Emotion(String);

impl Emotion {
    pub fn as_str(&self) -> &str {
        &self.0
    }
}

impl From<&str> for Emotion {
    fn from(value: &str) -> Self {
        Self(value.to_uppercase())
    }
}

impl From<String> for Emotion {
    fn from(mut value: String) -> Self {
        value.make_ascii_uppercase();
        Self(value)
    }
}

impl Default for Emotion {
    fn default() -> Self {
        Self(String::from("DEFAULT"))
    }
}

#[cfg(feature = "rollback_mlua")]
use rollback_mlua as mlua;

#[cfg(feature = "rollback_mlua")]
impl<'lua> mlua::FromLua<'lua> for Emotion {
    fn from_lua(lua_value: mlua::Value<'lua>, _: &'lua mlua::Lua) -> mlua::Result<Self> {
        let mlua::Value::String(id) = lua_value else {
            return Err(mlua::Error::FromLuaConversionError {
                from: lua_value.type_name(),
                to: "Emotion",
                message: None,
            });
        };

        Ok(Self(id.to_str()?.to_string()))
    }
}

#[cfg(feature = "rollback_mlua")]
impl<'lua> mlua::ToLua<'lua> for &Emotion {
    fn to_lua(self, lua: &'lua mlua::Lua) -> mlua::Result<mlua::Value<'lua>> {
        lua.create_string(&self.0).map(mlua::Value::String)
    }
}
