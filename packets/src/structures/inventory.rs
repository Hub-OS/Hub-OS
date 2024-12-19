use indexmap::IndexMap;
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, PartialEq, Debug)]
pub struct ItemDefinition {
    pub name: String,
    pub description: String,
    pub consumable: bool,
    pub sort_key: usize,
}

#[derive(Default)]
pub struct Inventory {
    items: IndexMap<String, usize>,
}

impl Inventory {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn items(&self) -> impl Iterator<Item = (&String, &usize)> {
        self.items.iter()
    }

    pub fn give_item(&mut self, item_id: &str, amount: isize) {
        self.items
            .entry(item_id.to_string())
            .and_modify(|count| *count = (*count as isize + amount).max(0) as _)
            .or_insert(amount.max(0) as _);
    }

    pub fn count_item(&self, item_id: &str) -> usize {
        self.items.get(item_id).cloned().unwrap_or_default()
    }

    pub fn item_registered(&self, item_id: &str) -> bool {
        self.items.iter().any(|(id, _)| id == item_id)
    }
}
