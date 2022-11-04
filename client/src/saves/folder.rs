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

    // todo: handle card classes, likely will need to take in game_io and namespace for resolving card classes
    pub fn shuffle(&mut self, rng: &mut impl rand::Rng) {
        use rand::seq::SliceRandom;

        self.cards.shuffle(rng);
    }
}
