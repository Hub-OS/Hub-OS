use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, Deserialize, Serialize)]
pub struct ShopItem {
    pub id: Option<String>,
    pub name: String,
    pub price_text: String,
}

impl ShopItem {
    pub fn id(&self) -> &str {
        self.id.as_ref().unwrap_or(&self.name)
    }
}
