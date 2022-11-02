#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct GenerationalIndex {
    pub index: usize,
    pub generation: usize,
}

impl GenerationalIndex {
    pub fn new(index: usize, generation: usize) -> Self {
        Self { index, generation }
    }

    pub fn tree_root() -> Self {
        Self::new(0, 0)
    }
}

impl From<GenerationalIndex> for generational_arena::Index {
    fn from(index: GenerationalIndex) -> Self {
        generational_arena::Index::from_raw_parts(index.index, index.generation as u64)
    }
}

impl From<generational_arena::Index> for GenerationalIndex {
    fn from(index: generational_arena::Index) -> Self {
        let (index, generation) = index.into_raw_parts();
        Self::new(index, generation as usize)
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

        Ok(Self::new(
            (number >> 32) as usize,
            (number & 0xFFFFFFFF) as usize,
        ))
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for GenerationalIndex {
    fn to_lua(self, _lua: &'lua rollback_mlua::Lua) -> rollback_mlua::Result<rollback_mlua::Value> {
        let n = ((self.index as i64) << 32) | (self.generation as i64 & 0xFFFFFFFF);
        Ok(rollback_mlua::Value::Integer(n))
    }
}
