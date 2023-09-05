use rollback_mlua::LuaSerdeExt;
use serde::{Deserialize, Serialize};

#[derive(Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum PackageNamespace {
    #[default]
    Local,
    Server,
    Netplay(usize),
}

impl PackageNamespace {
    pub fn is_netplay(self) -> bool {
        matches!(self, PackageNamespace::Netplay(_))
    }

    pub fn find_with_overrides<T, F>(self, mut callback: F) -> Option<T>
    where
        F: FnMut(Self) -> Option<T>,
    {
        match self {
            PackageNamespace::Netplay(_) => callback(PackageNamespace::Server)
                .or_else(|| callback(self))
                .or_else(|| callback(PackageNamespace::Local)),
            PackageNamespace::Server | PackageNamespace::Local => {
                callback(PackageNamespace::Server).or_else(|| callback(PackageNamespace::Local))
            }
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

impl<'lua> rollback_mlua::IntoLua<'lua> for PackageNamespace {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        lua.to_value(&self)
    }
}
