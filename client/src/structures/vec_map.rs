#[derive(Clone, PartialEq, Eq)]
pub struct VecMap<K, V> {
    list: Vec<(K, V)>,
}

impl<K, V> Default for VecMap<K, V> {
    fn default() -> Self {
        Self {
            list: Default::default(),
        }
    }
}

impl<K, V> VecMap<K, V>
where
    K: PartialEq,
{
    pub fn is_empty(&self) -> bool {
        self.list.is_empty()
    }

    pub fn get(&self, key: &K) -> Option<&V> {
        self.list
            .iter()
            .find(|(k, _)| key == k)
            .map(|(_, value)| value)
    }

    pub fn insert(&mut self, key: K, value: V) {
        if let Some(stored) = self
            .list
            .iter_mut()
            .find(|(f, _)| key == *f)
            .map(|(_, duration)| duration)
        {
            *stored = value;
        } else {
            self.list.push((key, value));
        };
    }

    pub fn iter(&self) -> impl Iterator<Item = &(K, V)> {
        self.list.iter()
    }

    pub fn from_unique_vec(v: Vec<(K, V)>) -> Self {
        Self { list: v }
    }
}

impl<'lua, K, V> VecMap<K, V>
where
    K: rollback_mlua::FromLua<'lua> + rollback_mlua::IntoLua<'lua> + Copy,
    V: rollback_mlua::FromLua<'lua> + rollback_mlua::IntoLua<'lua> + Copy,
{
    pub fn from_lua_table(table: rollback_mlua::Table<'lua>) -> Self {
        let durations = table.pairs::<K, V>().flatten().collect();

        Self { list: durations }
    }

    pub fn to_lua_table(
        &self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Table<'lua>> {
        let table = lua.create_table()?;

        for (key, value) in &self.list {
            table.raw_set(*key, *value)?;
        }

        Ok(table)
    }
}
