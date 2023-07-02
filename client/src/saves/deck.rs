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
    pub fn shuffle(&mut self, game_io: &GameIO, rng: &mut impl rand::Rng, namespace: PackageNamespace) {
        use rand::seq::SliceRandom;
        let globals = game_io.resource::<Globals>().unwrap();
        self.cards.shuffle(rng);
        // If the deck is larger than 10 cards
        if self.cards.len() < 10 {
            return;
        }
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
            loop {
                let rand_slot = rng.gen_range(9..self.cards.len());
                let rand_card = &self.cards[rand_slot];
                let Some(rand_package) = globals.card_packages.package_or_fallback(namespace, &rand_card.package_id) else {
                            continue;
                        };
                if rand_package.card_properties.card_class != CardClass::Giga {
                    self.cards.swap(count, rand_slot);
                    break;
                };
            }
        }
    }
}
