use super::{Globals, Restrictions};
use crate::bindable::CardClass;
use crate::packages::{
    AugmentPackage, CardPackage, PackageManager, PackageNamespace, PlayerPackage,
};
use crate::saves::{Card, Deck};
use framework::prelude::GameIO;
use packets::structures::PackageId;
use serde::Deserialize;
use std::collections::{HashMap, HashSet};

#[derive(Clone, Deserialize)]
#[serde(default)]
pub struct DeckRestrictions {
    pub required_total: usize,
    pub mega_limit: usize,
    pub giga_limit: usize,
    pub dark_limit: usize,
}

impl Default for DeckRestrictions {
    fn default() -> Self {
        Self {
            required_total: 30,
            mega_limit: 5,
            giga_limit: 1,
            dark_limit: 3,
        }
    }
}

impl DeckRestrictions {
    pub fn apply_augments<'a>(
        &mut self,
        player: Option<&PlayerPackage>,
        augments: impl Iterator<Item = (&'a AugmentPackage, usize)>,
    ) {
        let mut mega_boost = 0;
        let mut giga_boost = 0;
        let mut dark_boost = 0;
        let mut deck_boost = 0;

        if let Some(player) = player {
            mega_boost = player.mega_boost as isize;
            giga_boost = player.giga_boost as isize;
            dark_boost = player.dark_boost as isize;
            deck_boost = player.deck_boost as isize;
        }

        for (package, level) in augments {
            mega_boost += package.mega_boost as isize * level as isize;
            giga_boost += package.giga_boost as isize * level as isize;
            dark_boost += package.dark_boost as isize * level as isize;
            deck_boost += package.deck_boost as isize * level as isize;
        }

        self.mega_limit = (mega_boost + self.mega_limit as isize).max(0) as usize;
        self.giga_limit = (giga_boost + self.giga_limit as isize).max(0) as usize;
        self.dark_limit = (dark_boost + self.dark_limit as isize).max(0) as usize;
        self.required_total = (deck_boost + self.required_total as isize).max(0) as usize;
    }

    /// Uses validate_cards and checks regular card validity
    pub fn validate_deck(
        &self,
        game_io: &GameIO,
        namespace: PackageNamespace,
        deck: &Deck,
    ) -> DeckValidity {
        let mut deck_validity = self.validate_cards(game_io, namespace, deck.cards.iter());

        if !Self::validate_regular_card(game_io, namespace, &deck_validity, deck) {
            deck_validity.valid_regular = false;
            deck_validity.is_valid = false;
        }

        deck_validity
    }

    fn validate_regular_card(
        game_io: &GameIO,
        namespace: PackageNamespace,
        deck_validity: &DeckValidity,
        deck: &Deck,
    ) -> bool {
        let Some(i) = deck.regular_index else {
            return true;
        };

        let Some(card) = deck.cards.get(i) else {
            return false;
        };

        let globals = Globals::from_resources(game_io);

        let Some(package) = globals.card_packages.package(namespace, &card.package_id) else {
            return false;
        };

        package.regular_allowed && deck_validity.is_card_valid(card)
    }

    /// Does not account for regular card
    pub fn validate_cards<'a>(
        &self,
        game_io: &GameIO,
        namespace: PackageNamespace,
        cards: impl Iterator<Item = &'a Card>,
    ) -> DeckValidity {
        let card_packages = &Globals::from_resources(game_io).card_packages;
        let mut deck_validator = DeckValidator::new(game_io, namespace, self);
        let mut invalid_cards = HashSet::<Card>::new();

        let cards: Vec<&Card> = cards.collect();

        deck_validator.process_cards(&cards);

        for &card in &cards {
            if deck_validator.is_card_valid(card) {
                continue;
            }

            if card_packages
                .package_or_fallback(namespace, &card.package_id)
                .is_some()
            {
                continue;
            }

            invalid_cards.insert(card.clone());
        }

        DeckValidity {
            is_valid: self.required_total == deck_validator.total_cards && invalid_cards.is_empty(),
            valid_regular: true,
            invalid_cards,
        }
    }
}

#[derive(Default, Clone)]
pub struct DeckValidity {
    is_valid: bool,
    valid_regular: bool,
    invalid_cards: HashSet<Card>,
}

impl DeckValidity {
    pub fn is_valid(&self) -> bool {
        self.is_valid
    }

    pub fn is_regular_valid(&self) -> bool {
        self.valid_regular
    }

    pub fn is_card_valid(&self, card: &Card) -> bool {
        !self.invalid_cards.contains(card)
    }
}

struct DeckValidator<'a> {
    game_io: &'a GameIO,
    card_packages: &'a PackageManager<CardPackage>,
    restrictions: &'a Restrictions,
    deck_restrictions: &'a DeckRestrictions,
    namespace: PackageNamespace,
    total_cards: usize,
    mega_count: usize,
    giga_count: usize,
    dark_count: usize,
    id_counts: HashMap<&'a PackageId, usize>,
    card_counts: HashMap<&'a Card, usize>,
}

impl<'a> DeckValidator<'a> {
    pub fn new(
        game_io: &'a GameIO,
        namespace: PackageNamespace,
        deck_restrictions: &'a DeckRestrictions,
    ) -> Self {
        let globals = Globals::from_resources(game_io);

        Self {
            game_io,
            card_packages: &globals.card_packages,
            restrictions: &globals.restrictions,
            deck_restrictions,
            namespace,
            total_cards: 0,
            mega_count: 0,
            giga_count: 0,
            dark_count: 0,
            id_counts: HashMap::new(),
            card_counts: HashMap::new(),
        }
    }

    fn process_cards(&mut self, cards: &[&'a Card]) {
        for card in cards {
            self.process_card(card);
        }
    }

    fn process_card(&mut self, card: &'a Card) {
        self.total_cards += 1;

        let Some(package) = self.card_packages.package(self.namespace, &card.package_id) else {
            return;
        };

        // update card class counts
        match package.card_properties.card_class {
            CardClass::Mega => self.mega_count += 1,
            CardClass::Giga => self.giga_count += 1,
            CardClass::Dark => self.dark_count += 1,
            _ => {}
        }

        // update package specific count
        (self.id_counts)
            .entry(&package.package_info.id)
            .and_modify(|count| *count += 1)
            .or_insert(1);

        // update card count
        (self.card_counts)
            .entry(card)
            .and_modify(|count| *count += 1)
            .or_insert(1);
    }

    fn is_card_valid(&self, card: &Card) -> bool {
        // test package
        let Some(package) = self.card_packages.package(self.namespace, &card.package_id) else {
            return true;
        };

        // test class
        let class_valid = match package.card_properties.card_class {
            CardClass::Mega => self.mega_count <= self.deck_restrictions.mega_limit,
            CardClass::Giga => self.giga_count <= self.deck_restrictions.giga_limit,
            CardClass::Dark => self.dark_count <= self.deck_restrictions.dark_limit,
            _ => true,
        };

        if !class_valid {
            return false;
        }

        // test package count
        let package_count = self
            .id_counts
            .get(&package.package_info.id)
            .cloned()
            .unwrap_or_default();

        if package_count > package.limit {
            return false;
        }

        // test code count
        let count = self.card_counts.get(card).cloned().unwrap_or(0);

        if count > self.restrictions.card_count(self.game_io, card) {
            return false;
        }

        // test hash
        let package_triplet = package.package_info.triplet();
        let hashes_valid = self
            .restrictions
            .validate_package_tree(self.game_io, package_triplet);

        if !hashes_valid {
            return false;
        }

        true
    }
}
