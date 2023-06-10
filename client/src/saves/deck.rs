use crate::{bindable::CardClass, packages::PackageNamespace, resources::Globals, saves::Card};
use framework::prelude::GameIO;
use rand::{thread_rng, Rng};
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
        if self.cards.len() >= 10 {
            println!("count is greater than or equal to: {}", 10);
            for count in 0..9 {
                if is_shuffled == true {
                    is_shuffled = false;
                }
                let card_item = &self.cards[count];
                let Some(package) = globals.card_packages.package_or_fallback(PackageNamespace::Local, &card_item.package_id) else {
                    continue;
                };
                if package.card_properties.card_class == CardClass::Giga {
                    println!("card is giga class; attempting to hotswap.");
                    while is_shuffled == false {
                        let rand_slot = thread_rng().gen_range(10..self.cards.len()-1);
                        let rand_card = &self.cards[rand_slot];
                        let Some(rand_package) = globals.card_packages.package_or_fallback(PackageNamespace::Local, &rand_card.package_id) else {
                            continue;
                        };
                        if rand_package.card_properties.card_class != CardClass::Giga {
                            println!("found non-giga card; swapping...");
                            self.cards.swap(count, rand_slot);
                            is_shuffled = true;
                        };
                    }
                }
            }
        }
    }
}
