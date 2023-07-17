use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, PartialEq, Debug)]
pub struct ItemDefinition {
    pub name: String,
    pub description: String,
    pub consumable: bool,
}

#[derive(Default)]
pub struct Inventory {
    items: Vec<(String, usize)>,
}

impl Inventory {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn items(&self) -> impl Iterator<Item = &(String, usize)> {
        self.items.iter()
    }

    pub fn give_item(&mut self, item_id: &str, count: isize) {
        let item_iter = self.items.iter_mut();

        if let Some((_, (_, c))) = item_iter.enumerate().find(|(_, (id, _))| id == item_id) {
            *c = (*c as isize + count).max(0) as usize;
        } else if count > 0 {
            self.items.push((item_id.to_string(), count as usize));
        }
    }

    pub fn count_item(&self, item_id: &str) -> usize {
        self.items
            .iter()
            .find(|(id, _)| id == item_id)
            .map(|(_, count)| *count)
            .unwrap_or_default()
    }

    pub fn item_registered(&self, item_id: &str) -> bool {
        self.items.iter().any(|(id, _)| id == item_id)
    }
}
