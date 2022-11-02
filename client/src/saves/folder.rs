use crate::saves::Card;
use serde::{Deserialize, Serialize};

#[derive(Default, Clone, Serialize, Deserialize)]
pub struct Folder {
    pub name: String,
    pub cards: Vec<Card>,
}

impl Folder {
    pub const NAME_MAX_LEN: usize = 8;

    pub fn new(name: String) -> Self {
        Folder {
            name,
            cards: Vec::new(),
        }
    }
}
