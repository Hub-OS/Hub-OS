use crate::bindable::CardClass;
use crate::packages::PackageNamespace;
use crate::resources::{DeckRestrictions, Globals};
use crate::saves::Card;
use framework::prelude::GameIO;
use packets::structures::PackageId;
use serde::{Deserialize, Serialize};

#[derive(Default, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Deck {
    pub name: String,
    pub cards: Vec<Card>,
    pub regular_index: Option<usize>,
}

impl Deck {
    pub const NAME_MAX_LEN: usize = 8;

    pub fn new(name: String) -> Self {
        Deck {
            name,
            cards: Vec::new(),
            regular_index: None,
        }
    }

    pub fn conform(
        &mut self,
        game_io: &GameIO,
        namespace: PackageNamespace,
        deck_restrictions: &DeckRestrictions,
    ) {
        let deck_validity = deck_restrictions.validate_deck(game_io, namespace, self);

        if let Some(index) = self.regular_index {
            let card = &self.cards[index];

            if deck_validity.is_card_valid(card) {
                self.cards.swap(0, index);
                self.regular_index = Some(0);
            } else {
                self.regular_index = None;
            }
        }

        // preserve only valid cards
        self.cards.retain(|card| deck_validity.is_card_valid(card));

        // fill the rest of the deck
        let default_card = Card {
            package_id: PackageId::default(),
            code: String::from("!"),
        };

        let new_len = deck_restrictions.required_total;
        self.cards.resize(new_len, default_card);
    }

    pub fn shuffle(
        &mut self,
        game_io: &GameIO,
        rng: &mut impl rand::Rng,
        namespace: PackageNamespace,
    ) {
        use rand::seq::SliceRandom;

        // take out the regular card
        let regular_card = self.regular_index.map(|index| self.cards.remove(index));

        // shuffle
        self.cards.shuffle(rng);

        // put the regular card back in at the start
        if let Some(card) = regular_card {
            self.cards.insert(0, card);
        }

        // If the deck is less than 10 cards, don't try to giga shuffle.
        if self.cards.len() < 10 {
            return;
        }

        let globals = game_io.resource::<Globals>().unwrap();

        let mut non_giga_vec = Vec::new();
        // Cycle every card past the required 10 to count non-gigas
        for i in 10..self.cards.len() {
            let non_giga_card = &self.cards[i];
            let Some(package) = globals
                .card_packages
                .package_or_override(namespace, &non_giga_card.package_id)
            else {
                continue;
            };
            if package.card_properties.card_class == CardClass::Giga {
                continue;
            }
            non_giga_vec.push(i);
        }

        // Do not proceed without non-giga
        if non_giga_vec.is_empty() {
            return;
        }

        // Shuffle to obtain random movement order
        non_giga_vec.shuffle(rng);

        // Cycle the initial 10 starting at index 0
        for i in 0..=9 {
            // Get the card we're on, skip loop if it's blank somehow
            let card = &self.cards[i];
            let Some(package) = globals
                .card_packages
                .package_or_override(namespace, &card.package_id)
            else {
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
            self.cards.swap(i, swap_index);
        }
    }
}
