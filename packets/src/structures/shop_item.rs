use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, Deserialize, Serialize)]
pub struct ShopItem {
    pub name: String,
    pub description: String,
    pub price: u32,
}

impl ShopItem {
    pub fn calc_size(&self) -> usize {
        (std::mem::size_of::<String>() + self.name.len())
            + (std::mem::size_of::<String>() + self.description.len())
            + (std::mem::size_of::<u32>()) // price
    }
}
