use crate::Restrictions;
use crate::bindable::CardClass;
use crate::packages::PackageNamespace;
use crate::resources::{DeckRestrictions, Globals};
use crate::saves::Card;
use framework::prelude::GameIO;
use packets::structures::{PackageCategory, PackageId, Uuid};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use unicode_segmentation::UnicodeSegmentation;

#[derive(Default, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Deck {
    pub name: String,
    pub cards: Vec<Card>,
    pub regular_index: Option<usize>,
    #[serde(default = "Uuid::new_v4")]
    pub uuid: Uuid,
    #[serde(default)]
    pub update_time: u64,
}

impl Deck {
    pub const NAME_MAX_LEN: usize = 8;

    pub fn new(name: String) -> Self {
        Deck {
            name,
            cards: Vec::new(),
            regular_index: None,
            uuid: Uuid::new_v4(),
            update_time: 0,
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
            if deck_validity.is_regular_valid() {
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
            code: String::from("?"),
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
                .package_or_fallback(namespace, &non_giga_card.package_id)
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

        // Cycle the initial 10, skipping the first index if it's a regular card
        let initial_index = if self.regular_index.is_some() { 1 } else { 0 };

        for i in initial_index..10 {
            // Get the card we're on, skip loop if it's blank somehow
            let card = &self.cards[i];
            let Some(package) = globals
                .card_packages
                .package_or_fallback(namespace, &card.package_id)
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

    pub fn resolve_relevant_recipes(
        &self,
        game_io: &GameIO,
        restrictions: &Restrictions,
        ns: PackageNamespace,
    ) -> Vec<PackageId> {
        let globals = game_io.resource::<Globals>().unwrap();
        let mut results = Vec::new();

        for card in &self.cards {
            let Some(package) = globals.card_packages.package(ns, &card.package_id) else {
                continue;
            };

            for id in globals.card_recipes.related_output(ns, package) {
                if results.contains(&id) {
                    continue;
                }

                let triplet = (PackageCategory::Card, ns, id.clone());

                if !restrictions.validate_package_tree(game_io, triplet) {
                    continue;
                }

                results.push(id);
            }
        }

        results
    }

    pub fn export_string(&self, game_io: &GameIO) -> String {
        // condense cards
        let globals = game_io.resource::<Globals>().unwrap();
        let card_packages = &globals.card_packages;
        let mut card_map = HashMap::new();

        for card in &self.cards {
            if let Some((_, count)) = card_map.get_mut(card) {
                *count += 1;
                continue;
            }

            let Some(package) = card_packages.package(PackageNamespace::Local, &card.package_id)
            else {
                continue;
            };

            card_map.insert(card, (package, 1));
        }

        // sort cards
        let mut card_list: Vec<_> = card_map.into_iter().collect();

        card_list.sort_by_key(|(card, (package, _))| {
            (
                package.card_properties.card_class,
                &card.package_id,
                &card.code,
            )
        });

        // generate output
        use std::fmt::Write;

        let mut text = String::new();

        for (card, (package, count)) in card_list {
            let id = &card.package_id;
            let name = &package.card_properties.short_name;
            let code = &card.code;

            let _ = writeln!(&mut text, "{count} {name:<8.8} {code} {id}");
        }

        text
    }

    pub fn import_string(text: String) -> Option<Deck> {
        let mut deck = Deck::default();

        for line in text.lines() {
            if line.is_empty() {
                continue;
            }

            // read count
            let count_end = line.find(" ")?;
            let count: u8 = line[0..count_end].parse().ok()?;

            // read name
            let name_start = count_end + 1;
            let name_len = text[name_start..]
                .grapheme_indices(true)
                .nth(8)
                .map(|(i, _)| i)
                .unwrap_or(line.len());

            let name_end = name_start + name_len;

            if line.len() < name_end + 1 {
                // must have room for the name and a space for the next read
                return None;
            }

            // read code
            let code_start = name_end + 1;
            let code_end = line[code_start..].find(" ")? + code_start;
            let code = &line[code_start..code_end];

            // read id
            let id = &line[code_end + 1..];

            let card = Card {
                package_id: id.into(),
                code: code.into(),
            };

            deck.cards.extend(std::iter::repeat_n(card, count as _));
        }

        Some(deck)
    }
}

#[cfg(test)]
mod test {
    use crate::saves::Deck;

    #[test]
    fn import() {
        const PROTO_MAN_EX: &str = "2 ProtoMn B BattleNetwork6.Class02.Mega.005.ProtoEX";

        let deck = Deck::import_string(PROTO_MAN_EX.to_string()).unwrap();

        for card in &deck.cards {
            assert_eq!(card.code.as_str(), "B");
            assert_eq!(
                card.package_id.as_str(),
                "BattleNetwork6.Class02.Mega.005.ProtoEX"
            );
        }

        assert_eq!(deck.cards.len(), 2)
    }
}
