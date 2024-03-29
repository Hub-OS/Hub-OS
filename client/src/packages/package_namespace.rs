use serde::{Deserialize, Serialize};

#[derive(
    Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize, PartialOrd, Ord,
)]
pub enum PackageNamespace {
    RecordingServer,
    Server,
    #[default]
    Local,
    Netplay(u8),
    BuiltIn,
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
                .or_else(|| callback(PackageNamespace::Local))
                .or_else(|| callback(PackageNamespace::BuiltIn)),
            PackageNamespace::RecordingServer => callback(PackageNamespace::RecordingServer),
            PackageNamespace::Server | PackageNamespace::Local | PackageNamespace::BuiltIn => {
                callback(PackageNamespace::Server)
                    .or_else(|| callback(PackageNamespace::Local))
                    .or_else(|| callback(PackageNamespace::BuiltIn))
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
            3 => PackageNamespace::BuiltIn,
            _ => PackageNamespace::Netplay((number - 4) as u8),
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
            PackageNamespace::BuiltIn => 3,
            PackageNamespace::Netplay(i) => i as rollback_mlua::Integer + 4,
        };

        Ok(rollback_mlua::Value::Integer(number))
    }
}
