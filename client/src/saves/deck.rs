use crate::{bindable::CardClass, packages::PackageNamespace, resources::Globals, saves::Card};
use framework::prelude::GameIO;
use serde::{Deserialize, Serialize};

#[derive(Default, Clone, Serialize, Deserialize)]
pub struct Deck {
    pub name: String,
    pub cards: Vec<Card>,
}

impl Deck {
    pub const NAME_MAX_LEN: usize = 8;

    pub fn new(name: String) -> Self {
        Deck {
            name,
            cards: Vec::new(),
        }
    }

    // todo: handle card classes, likely will need to take in game_io and namespace for resolving card classes
    pub fn shuffle(
        &mut self,
        game_io: &GameIO,
        rng: &mut impl rand::Rng,
        namespace: PackageNamespace,
    ) {
        use rand::seq::SliceRandom;
        let globals = game_io.resource::<Globals>().unwrap();
        self.cards.shuffle(rng);
        
        // If the deck is less than 20 cards, don't try to giga shuffle.
        if self.cards.len() < 10 {
            return;
        }
        
        let mut non_giga_vec = Vec::new();
        // Cycle every card past the required 10 to count non-gigas
        for count in 10..self.cards.len() {
            let non_giga_card = &self.cards[count];
            let Some(package) = globals.card_packages.package_or_fallback(namespace, &non_giga_card.package_id) else {
                continue;
            };
            if package.card_properties.card_class == CardClass::Giga {
                continue;
            }
            non_giga_vec.push(count);
        }
        
        // Do not proceed without non-giga
        if non_giga_vec.is_empty() {
            return;
        }

        // Shuffle to obtain random movement order
        non_giga_vec.shuffle(rng);

        // Cycle the initial 10 starting at index 0
        for count in 0..=9 {
            // Get the card we're on, skip loop if it's blank somehow
            let card = &self.cards[count];
            let Some(package) = globals.card_packages.package_or_fallback(namespace, &card.package_id) else {
                continue;
            };

            // If it's not giga, don't proceed.
            if package.card_properties.card_class != CardClass::Giga {
                continue;
            }
            
            // Proceed only if it is valid
            let Some(swap_index) = non_giga_vec.pop() else {
                continue;
            };
            
            // Swap the cards.
            self.cards.swap(count, swap_index);
        }
    }
}
