use serde::{Deserialize, Serialize};
use std::borrow::Cow;

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Emotion(Cow<'static, str>);

impl Emotion {
    pub fn as_str(&self) -> &str {
        &self.0
    }
}

impl From<&str> for Emotion {
    fn from(value: &str) -> Self {
        Self(Cow::Owned(value.to_uppercase()))
    }
}

impl From<String> for Emotion {
    fn from(mut value: String) -> Self {
        value.make_ascii_uppercase();
        Self(Cow::Owned(value))
    }
}

impl Default for Emotion {
    fn default() -> Self {
        Self(Cow::Borrowed("DEFAULT"))
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

        Ok(Self(Cow::Owned(id.to_str()?.to_string())))
    }
}

#[cfg(feature = "rollback_mlua")]
impl<'lua> mlua::ToLua<'lua> for &Emotion {
    fn to_lua(self, lua: &'lua mlua::Lua) -> mlua::Result<mlua::Value<'lua>> {
        lua.create_string(self.as_str()).map(mlua::Value::String)
    }
}
