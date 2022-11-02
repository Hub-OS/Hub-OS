use crate::bindable::*;

#[derive(Clone)]
pub struct Character {
    pub rank: CharacterRank,
    pub cards: Vec<CardProperties>,
}

impl Character {
    pub fn new(rank: CharacterRank) -> Self {
        Self {
            rank,
            cards: Vec::new(),
        }
    }
}
