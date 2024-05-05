use serde::{Deserialize, Serialize};

#[derive(
    Default, Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize, PartialOrd, Ord,
)]
pub enum PackageNamespace {
    #[default]
    BuiltIn,
    Local,
    Server,
    Netplay(u8),
    RecordingBuiltIn,
    RecordingLocal,
    RecordingServer,
}

impl PackageNamespace {
    pub fn is_netplay(self) -> bool {
        matches!(self, PackageNamespace::Netplay(_))
    }

    pub fn is_recording(self) -> bool {
        matches!(
            self,
            PackageNamespace::Netplay(_)
                | PackageNamespace::RecordingBuiltIn
                | PackageNamespace::RecordingLocal
                | PackageNamespace::RecordingServer
        )
    }

    pub fn strip_recording(self) -> Self {
        match self {
            PackageNamespace::RecordingBuiltIn => PackageNamespace::BuiltIn,
            PackageNamespace::RecordingLocal => PackageNamespace::Local,
            PackageNamespace::RecordingServer => PackageNamespace::Server,
            _ => self,
        }
    }

    pub fn prefix_recording(self) -> Self {
        match self {
            PackageNamespace::BuiltIn => PackageNamespace::RecordingBuiltIn,
            PackageNamespace::Local => PackageNamespace::RecordingLocal,
            PackageNamespace::Server => PackageNamespace::RecordingServer,
            _ => self,
        }
    }

    fn internal_find_with_fallback<T, F>(self, mut callback: F) -> Option<T>
    where
        F: FnMut(Self) -> Option<T>,
    {
        match self {
            PackageNamespace::Server => callback(PackageNamespace::Server)
                .or_else(|| callback(PackageNamespace::Local))
                .or_else(|| callback(PackageNamespace::BuiltIn)),
            PackageNamespace::Local | PackageNamespace::BuiltIn => {
                callback(PackageNamespace::Local).or_else(|| callback(PackageNamespace::BuiltIn))
            }
            _ => unreachable!(),
        }
    }

    pub fn find_with_fallback<T, F>(self, mut callback: F) -> Option<T>
    where
        F: FnMut(Self) -> Option<T>,
    {
        match self {
            PackageNamespace::Netplay(_) => callback(self)
                .or_else(|| {
                    Self::internal_find_with_fallback(PackageNamespace::Local, |ns| {
                        callback(ns.prefix_recording())
                    })
                })
                .or_else(|| {
                    Self::internal_find_with_fallback(PackageNamespace::Local, &mut callback)
                }),
            PackageNamespace::BuiltIn | PackageNamespace::Local | PackageNamespace::Server => {
                Self::internal_find_with_fallback(self, &mut callback)
            }
            PackageNamespace::RecordingBuiltIn
            | PackageNamespace::RecordingLocal
            | PackageNamespace::RecordingServer => {
                Self::internal_find_with_fallback(self.strip_recording(), |ns| {
                    callback(ns.prefix_recording())
                })
            }
        }
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
            0 => PackageNamespace::BuiltIn,
            1 => PackageNamespace::Local,
            2 => PackageNamespace::Server,
            3 => PackageNamespace::RecordingBuiltIn,
            4 => PackageNamespace::RecordingLocal,
            5 => PackageNamespace::RecordingServer,
            _ => PackageNamespace::Netplay((number - 6) as u8),
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
            PackageNamespace::BuiltIn => 0,
            PackageNamespace::Local => 1,
            PackageNamespace::Server => 2,
            PackageNamespace::RecordingBuiltIn => 3,
            PackageNamespace::RecordingLocal => 4,
            PackageNamespace::RecordingServer => 5,
            PackageNamespace::Netplay(i) => i as rollback_mlua::Integer + 6,
        };

        Ok(rollback_mlua::Value::Integer(number))
    }
}
