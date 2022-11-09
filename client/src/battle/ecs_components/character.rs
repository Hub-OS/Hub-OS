use crate::{bindable::*, packages::PackageNamespace};

#[derive(Clone)]
pub struct Character {
    pub rank: CharacterRank,
    pub cards: Vec<CardProperties>,
    pub namespace: PackageNamespace,
}

impl Character {
    pub fn new(rank: CharacterRank, namespace: PackageNamespace) -> Self {
        Self {
            rank,
            cards: Vec::new(),
            namespace,
        }
    }
}
