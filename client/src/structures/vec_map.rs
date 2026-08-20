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

    pub fn len(&self) -> usize {
        self.list.len()
    }

    pub fn contains(&self, key: &K) -> bool {
        self.list.iter().any(|(k, _)| key == k)
    }

    pub fn get(&self, key: &K) -> Option<&V> {
        self.list
            .iter()
            .find(|(k, _)| key == k)
            .map(|(_, value)| value)
    }

    pub fn get_mut(&mut self, key: &K) -> Option<&mut V> {
        self.list
            .iter_mut()
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
        }
    }

    pub fn swap_remove(&mut self, key: &K) -> Option<V> {
        let i = self.list.iter().position(|(f, _)| key == f)?;
        Some(self.list.swap_remove(i).1)
    }

    pub fn swap_remove_entry(&mut self, key: &K) -> Option<(K, V)> {
        let i = self.list.iter().position(|(f, _)| key == f)?;
        Some(self.list.swap_remove(i))
    }

    pub fn entry(&mut self, key: K) -> VecMapEntry<'_, K, V> {
        if let Some(index) = self.list.iter_mut().position(|(f, _)| key == *f) {
            VecMapEntry::Occupied(VecMapOccupiedEntry { map: self, index })
        } else {
            VecMapEntry::Vacant(VecMapVacantEntry { map: self, key })
        }
    }

    pub fn clear(&mut self) {
        self.list.clear();
    }

    pub fn retain(&mut self, mut f: impl FnMut(&K, &V) -> bool) {
        self.list.retain(|(k, v)| f(k, v))
    }

    pub fn retain_mut(&mut self, mut f: impl FnMut(&K, &mut V) -> bool) {
        self.list.retain_mut(|(k, v)| f(k, v))
    }

    pub fn iter(&self) -> impl Iterator<Item = &(K, V)> {
        self.list.iter()
    }

    pub fn iter_mut(&mut self) -> impl Iterator<Item = (&K, &mut V)> {
        self.list.iter_mut().map(|(key, value)| (&*key, value))
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

pub enum VecMapEntry<'a, K, V> {
    Occupied(VecMapOccupiedEntry<'a, K, V>),
    Vacant(VecMapVacantEntry<'a, K, V>),
}

impl<'a, K: PartialEq, V: Default> VecMapEntry<'a, K, V> {
    pub fn or_default(self) -> &'a mut V {
        match self {
            VecMapEntry::Occupied(occupied_entry) => {
                let index = occupied_entry.index;
                &mut occupied_entry.map.list[index].1
            }
            VecMapEntry::Vacant(vacant_entry) => {
                let map = vacant_entry.map;
                map.list.push((vacant_entry.key, Default::default()));
                &mut map.list.last_mut().unwrap().1
            }
        }
    }
}

pub struct VecMapOccupiedEntry<'a, K, V> {
    map: &'a mut VecMap<K, V>,
    index: usize,
}

// we could probably use unsafe for faster access, but i won't
impl<'a, K: PartialEq, V> VecMapOccupiedEntry<'a, K, V> {
    pub fn key(&self) -> &K {
        &self.map.list[self.index].0
    }

    pub fn get(&self) -> &V {
        &self.map.list[self.index].1
    }

    pub fn get_mut(&mut self) -> &mut V {
        &mut self.map.list[self.index].1
    }

    pub fn into_mut(self) -> &'a mut V {
        &mut self.map.list[self.index].1
    }

    pub fn insert(&mut self, mut value: V) -> V {
        std::mem::swap(&mut self.map.list[self.index].1, &mut value);
        value
    }

    pub fn swap_remove(&mut self) -> V {
        self.map.list.swap_remove(self.index).1
    }

    pub fn swap_remove_entry(&mut self) -> (K, V) {
        self.map.list.swap_remove(self.index)
    }
}

pub struct VecMapVacantEntry<'a, K, V> {
    map: &'a mut VecMap<K, V>,
    key: K,
}

// we could probably use unsafe for faster access, but i won't
impl<'a, K, V> VecMapVacantEntry<'a, K, V> {
    pub fn key(&self) -> &K {
        &self.key
    }

    pub fn insert(self, value: V) {
        self.map.list.push((self.key, value));
    }
}
