use serde::{Deserialize, Serialize};
use std::sync::Arc;

#[derive(Default, Clone, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct PackageId(Arc<str>);

impl Serialize for PackageId {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&self.0)
    }
}

struct PackageIdVisitor;

impl serde::de::Visitor<'_> for PackageIdVisitor {
    type Value = PackageId;

    fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        formatter.write_str("a string value")
    }

    fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        Ok(v.into())
    }

    fn visit_string<E>(self, v: String) -> Result<Self::Value, E>
    where
        E: serde::de::Error,
    {
        Ok(v.into())
    }
}

impl<'de> Deserialize<'de> for PackageId {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        deserializer.deserialize_str(PackageIdVisitor)
    }
}

impl PackageId {
    pub fn new_blank() -> Self {
        Self("".into())
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
        Self(id.into())
    }
}

impl From<String> for PackageId {
    fn from(id: String) -> Self {
        Self(id.into())
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

        Ok(Self(id.to_string_lossy().into()))
    }
}

#[cfg(feature = "rollback_mlua")]
impl<'lua> mlua::IntoLua<'lua> for PackageId {
    fn into_lua(self, lua: &'lua mlua::Lua) -> mlua::Result<mlua::Value<'lua>> {
        lua.create_string(&*self.0).map(mlua::Value::String)
    }
}
