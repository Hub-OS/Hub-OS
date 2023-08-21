#[derive(Debug, Hash, PartialOrd, Ord, PartialEq, Eq, Clone, Copy, Default)]
pub struct GenerationalIndex {
    pub index: usize,
    pub generation: u32,
}

impl GenerationalIndex {
    pub fn new(index: usize, generation: u32) -> Self {
        Self { index, generation }
    }

    pub fn tree_root() -> Self {
        Self::new(0, 0)
    }
}

unsafe impl slotmap::Key for GenerationalIndex {
    fn data(&self) -> slotmap::KeyData {
        slotmap::KeyData::from_ffi((*self).into())
    }
}

impl From<slotmap::KeyData> for GenerationalIndex {
    fn from(value: slotmap::KeyData) -> Self {
        Self::from(value.as_ffi())
    }
}

impl From<GenerationalIndex> for u64 {
    fn from(value: GenerationalIndex) -> Self {
        ((value.generation as u64) << 32) | (value.index as u64)
    }
}

impl From<u64> for GenerationalIndex {
    fn from(value: u64) -> Self {
        Self {
            index: (value & 0xFFFFFFFF) as usize,
            generation: (value >> 32) as u32,
        }
    }
}

impl From<GenerationalIndex> for i64 {
    fn from(value: GenerationalIndex) -> Self {
        let unsigned: u64 = value.into();
        unsigned as i64
    }
}

impl From<i64> for GenerationalIndex {
    fn from(value: i64) -> Self {
        Self::from(value as u64)
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for GenerationalIndex {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let number = match lua_value {
            rollback_mlua::Value::Integer(number) => number,
            rollback_mlua::Value::Table(table) => table.raw_get("#id")?,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "GenerationalIndex",
                    message: None,
                })
            }
        };

        Ok(number.into())
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for GenerationalIndex {
    fn to_lua(self, _lua: &'lua rollback_mlua::Lua) -> rollback_mlua::Result<rollback_mlua::Value> {
        let n = self.into();
        Ok(rollback_mlua::Value::Integer(n))
    }
}
