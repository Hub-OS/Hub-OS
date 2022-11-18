use rollback_mlua::LuaSerdeExt;
use serde::{Deserialize, Serialize};
use std::cmp::Ordering;

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum PackageNamespace {
    Local,
    Server,
    Remote(usize),
}

impl PackageNamespace {
    pub fn is_remote(self) -> bool {
        matches!(self, PackageNamespace::Remote(_))
    }

    pub fn find_with_fallback<T, F>(self, mut callback: F) -> Option<T>
    where
        F: FnMut(Self) -> Option<T>,
    {
        let result = callback(self);

        if result.is_some() {
            return result;
        }

        match self {
            PackageNamespace::Remote(_) => PackageNamespace::Server.find_with_fallback(callback),
            PackageNamespace::Server => callback(PackageNamespace::Local),
            PackageNamespace::Local => None,
        }
    }
}

impl Default for PackageNamespace {
    fn default() -> Self {
        PackageNamespace::Local
    }
}

impl PartialOrd for PackageNamespace {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        match (self, other) {
            (PackageNamespace::Local, PackageNamespace::Server | PackageNamespace::Remote(_))
            | (PackageNamespace::Server, PackageNamespace::Remote(_)) => {
                return Some(Ordering::Less);
            }
            (PackageNamespace::Server, PackageNamespace::Local)
            | (PackageNamespace::Remote(_), PackageNamespace::Local | PackageNamespace::Server) => {
                return Some(Ordering::Greater);
            }
            _ => Some(Ordering::Equal),
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for PackageNamespace {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        lua.from_value(lua_value)
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for PackageNamespace {
    fn to_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        lua.to_value(&self)
    }
}
