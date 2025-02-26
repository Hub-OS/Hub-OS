#[cfg(all(feature = "rollback_mlua", not(feature = "mlua")))]
use rollback_mlua as mlua;

#[cfg(feature = "mlua")]
use mlua;

#[macro_export]
macro_rules! create_generational_index {
    ($name: ident) => {
        #[derive(Hash, PartialOrd, Ord, PartialEq, Eq, Clone, Copy, Default)]
        pub struct $name {
            pub index: u32,
            pub generation: u32,
        }

        impl $name {
            pub fn new(index: u32, generation: u32) -> Self {
                Self { index, generation }
            }
        }

        impl std::fmt::Debug for $name {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                write!(f, "{}v{}", self.index, self.generation)
            }
        }

        unsafe impl slotmap::Key for $name {
            fn data(&self) -> slotmap::KeyData {
                slotmap::KeyData::from_ffi((*self).into())
            }
        }

        impl From<slotmap::KeyData> for $name {
            fn from(value: slotmap::KeyData) -> Self {
                Self::from(value.as_ffi())
            }
        }

        impl From<$name> for u64 {
            fn from(value: $name) -> Self {
                ((value.generation as u64) << 32) | (value.index as u64)
            }
        }

        impl From<u64> for $name {
            fn from(value: u64) -> Self {
                Self {
                    index: (value & 0xFFFFFFFF) as _,
                    generation: (value >> 32) as _,
                }
            }
        }

        impl From<$name> for i64 {
            fn from(value: $name) -> Self {
                let unsigned: u64 = value.into();
                unsigned as i64
            }
        }

        impl From<i64> for $name {
            fn from(value: i64) -> Self {
                Self::from(value as u64)
            }
        }

        impl<'lua> mlua::FromLua<'lua> for $name {
            fn from_lua(lua_value: mlua::Value<'lua>, _lua: &'lua mlua::Lua) -> mlua::Result<Self> {
                let mlua::Value::Integer(number) = lua_value else {
                    return Err(mlua::Error::FromLuaConversionError {
                        from: lua_value.type_name(),
                        to: "$name",
                        message: None,
                    });
                };

                Ok(number.into())
            }
        }

        impl<'lua> mlua::IntoLua<'lua> for $name {
            fn into_lua(self, _lua: &'lua mlua::Lua) -> mlua::Result<mlua::Value<'lua>> {
                let n = self.into();
                Ok(mlua::Value::Integer(n))
            }
        }
    };
}

macro_rules! impl_serde_for_generational_index {
    ($name: ident) => {
        impl serde::Serialize for $name {
            fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
            where
                S: serde::Serializer,
            {
                serializer.serialize_u64((*self).into())
            }
        }

        impl<'de> serde::Deserialize<'de> for $name {
            fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
            where
                D: serde::Deserializer<'de>,
            {
                let n = u64::deserialize(deserializer)?;
                Ok(Self::from(n))
            }
        }
    };
}

create_generational_index!(ActorId);
impl_serde_for_generational_index!(ActorId);
create_generational_index!(SpriteId);
impl_serde_for_generational_index!(SpriteId);
