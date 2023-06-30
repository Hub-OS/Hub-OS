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
    pub fn shuffle(&mut self, rng: &mut impl rand::Rng, game_io: &GameIO) {
        use rand::seq::SliceRandom;
        let globals = game_io.resource::<Globals>().unwrap();
        let mut is_shuffled = false;
        self.cards.shuffle(rng);
        // If the deck is larger than 10 cards
        if self.cards.len() >= 10 {
            // Cycle the initial 10 starting at index 0
            for count in 0..9 {
                // If we already shuffled, make sure to reset it
                if is_shuffled == true {
                    is_shuffled = false;
                }
                // Get the card we're on, skip loop if it's blank somehow
                let card_item = &self.cards[count];
                let Some(package) = globals.card_packages.package_or_fallback(PackageNamespace::Local, &card_item.package_id) else {
                    continue;
                };
                // If it's a giga class, loop until we can shuffle it somewhere safe by cycling it forward until we find a safe slot
                // that's greater than index 9 (aka, 10th card in the deck)
                if package.card_properties.card_class == CardClass::Giga {
                    while is_shuffled == false {
                        let rand_slot = rng.gen_range(9..self.cards.len());
                        let rand_card = &self.cards[rand_slot];
                        let Some(rand_package) = globals.card_packages.package_or_fallback(PackageNamespace::Local, &rand_card.package_id) else {
                            continue;
                        };
                        if rand_package.card_properties.card_class != CardClass::Giga {
                            self.cards.swap(count, rand_slot);
                            is_shuffled = true;
                        };
                    }
                }
            }
        }
    }
}
