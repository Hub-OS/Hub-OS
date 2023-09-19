use serde::{Deserialize, Serialize};

#[derive(
    Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize, PartialOrd, Ord,
)]
pub enum PackageNamespace {
    #[default]
    Local,
    Server,
    RecordingServer,
    Netplay(usize),
}

impl PackageNamespace {
    pub fn is_netplay(self) -> bool {
        matches!(self, PackageNamespace::Netplay(_))
    }

    pub fn is_recording(self) -> bool {
        matches!(
            self,
            PackageNamespace::Netplay(_) | PackageNamespace::RecordingServer
        )
    }

    pub fn find_with_overrides<T, F>(self, mut callback: F) -> Option<T>
    where
        F: FnMut(Self) -> Option<T>,
    {
        match self {
            PackageNamespace::Netplay(_) => callback(PackageNamespace::RecordingServer)
                .or_else(|| callback(PackageNamespace::Server))
                .or_else(|| callback(self))
                .or_else(|| callback(PackageNamespace::Local)),
            PackageNamespace::RecordingServer => callback(PackageNamespace::RecordingServer),
            PackageNamespace::Server | PackageNamespace::Local => {
                callback(PackageNamespace::Server).or_else(|| callback(PackageNamespace::Local))
            }
        }
    }

    pub fn has_override(self, ns: Self) -> bool {
        self.find_with_overrides(|test_ns| if test_ns == ns { Some(()) } else { None })
            .is_some()
    }

    pub fn into_server(self) -> Self {
        match self {
            PackageNamespace::RecordingServer => self,
            _ => PackageNamespace::Server,
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for PackageNamespace {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let number = match lua_value {
            rollback_mlua::Value::Integer(number) => number,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "PackageNamespace",
                    message: None,
                })
            }
        };

        let ns = match number {
            0 => PackageNamespace::Local,
            1 => PackageNamespace::Server,
            2 => PackageNamespace::RecordingServer,
            _ => PackageNamespace::Netplay(number as usize - 3),
        };

        Ok(ns)
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for PackageNamespace {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let number = match self {
            PackageNamespace::Local => 0,
            PackageNamespace::Server => 1,
            PackageNamespace::RecordingServer => 2,
            PackageNamespace::Netplay(i) => i as rollback_mlua::Integer + 3,
        };

        Ok(rollback_mlua::Value::Integer(number))
    }
}
