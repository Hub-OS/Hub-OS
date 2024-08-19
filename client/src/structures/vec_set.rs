#[derive(Clone)]
pub struct VecSet<T> {
    values: Vec<T>,
}

impl<T> Default for VecSet<T> {
    fn default() -> Self {
        Self {
            values: Default::default(),
        }
    }
}

impl<T> VecSet<T> {
    pub fn new() -> Self {
        Self { values: Vec::new() }
    }

    pub fn is_empty(&self) -> bool {
        self.values.is_empty()
    }

    pub fn clear(&mut self) {
        self.values.clear()
    }
}

impl<T: PartialEq> VecSet<T> {
    pub fn contains(&self, value: &T) -> bool {
        self.values.contains(value)
    }

    pub fn insert(&mut self, value: T) {
        if !self.contains(&value) {
            self.values.push(value)
        }
    }

    pub fn remove(&mut self, value: T) -> Option<T> {
        let index = self.values.iter().position(|v| *v == value)?;

        Some(self.values.swap_remove(index))
    }
}

impl<T: PartialEq> Extend<T> for VecSet<T> {
    fn extend<I: IntoIterator<Item = T>>(&mut self, iter: I) {
        for value in iter {
            self.insert(value);
        }
    }
}

impl<T: PartialEq, I: Iterator<Item = T>> From<I> for VecSet<T> {
    fn from(iter: I) -> Self {
        let mut set = Self::new();
        set.extend(iter);
        set
    }
}

impl<T> IntoIterator for VecSet<T> {
    type Item = T;
    type IntoIter = std::vec::IntoIter<T>;

    fn into_iter(self) -> Self::IntoIter {
        self.values.into_iter()
    }
}

impl<T> AsRef<[T]> for VecSet<T> {
    fn as_ref(&self) -> &[T] {
        &self.values
    }
}

impl<T> std::ops::Deref for VecSet<T> {
    type Target = [T];

    fn deref(&self) -> &Self::Target {
        &self.values
    }
}
