use super::{DeckRestrictions, Globals};
use crate::{packages::PackageNamespace, saves::Card};
use framework::prelude::GameIO;
use packets::structures::{BlockColor, FileHash, InstalledBlock, PackageCategory, PackageId};
use std::{
    borrow::Cow,
    collections::{HashMap, HashSet},
};

#[derive(Default)]
pub struct Restrictions {
    package_whitelist: HashSet<FileHash>,
    package_blacklist: HashSet<FileHash>,
    base_deck_restrictions: DeckRestrictions,
    owned_cards: HashMap<Card, usize>,
    owned_blocks: HashMap<(Cow<'static, PackageId>, BlockColor), usize>,
    owned_players: HashSet<PackageId>,
    // battle restrictions, used to generate initial BattleConfig
    // todo: not in use
    // pub status_durations: [FrameTime; 3],
    // pub intangibility_duration: FrameTime,
    // pub super_effective_multiplier: f32,
    // pub turn_limit: Option<u32>,
}

// impl Default for Restrictions {
//     fn default() -> Self {
//         Self {
//             package_whitelist: HashSet::new(),
//             package_blacklist: HashSet::new(),
//             owned_cards: HashMap::new(),
//             owned_blocks: HashMap::new(),
//             owned_players: HashSet::new(),
//             default_deck_restrictions: DeckRestrictions::default(),
//             status_durations: [90, 120, 150],
//             intangibility_duration: 120,
//             super_effective_multiplier: 2.0,
//             turn_limit: None,
//         }
//     }
// }

impl Restrictions {
    fn is_hash_allowed(&self, hash: &FileHash) -> bool {
        (self.package_whitelist.is_empty() || self.package_whitelist.contains(hash))
            && (self.package_blacklist.is_empty() || !self.package_blacklist.contains(hash))
    }

    pub fn base_deck_restrictions(&self) -> DeckRestrictions {
        self.base_deck_restrictions.clone()
    }

    pub fn card_count(&self, game_io: &GameIO, card: &Card) -> usize {
        if self.owned_cards.is_empty() {
            // use package info
            let globals = game_io.resource::<Globals>().unwrap();
            let card_packages = &globals.card_packages;

            card_packages
                .package_or_fallback(PackageNamespace::Local, &card.package_id)
                .map(|package| package.card_properties.limit)
                .unwrap_or(0)
        } else {
            // use self.owned_cards
            self.owned_cards.get(card).cloned().unwrap_or(0)
        }
    }

    pub fn block_count(&self, game_io: &GameIO, id: &PackageId, color: BlockColor) -> usize {
        if self.owned_blocks.is_empty() {
            // use package info
            let globals = game_io.resource::<Globals>().unwrap();
            let augment_packages = &globals.augment_packages;

            augment_packages
                .package_or_fallback(PackageNamespace::Local, id)
                .map(|package| {
                    package
                        .block_colors
                        .iter()
                        .filter(|&listed_color| *listed_color == color)
                        .count()
                })
                .unwrap_or(0)
        } else {
            // use self.owned_blocks
            self.owned_blocks
                .get(&(Cow::Borrowed(id), color))
                .cloned()
                .unwrap_or(0)
        }
    }

    pub fn owns_player(&self, id: &PackageId) -> bool {
        self.owned_players.is_empty() || self.owned_players.contains(id)
    }

    /// Returns true if the package and its dependencies are valid
    pub fn validate_package_tree(
        &self,
        game_io: &GameIO,
        triplet: (PackageCategory, PackageNamespace, PackageId),
    ) -> bool {
        let globals = game_io.resource::<Globals>().unwrap();
        let deps = globals.package_dependency_iter(std::iter::once(triplet));

        // test hashes while tracking dependencies
        let mut packages_hit = HashSet::new();
        let mut packages_expected = HashSet::new();

        for (info, _) in deps.iter() {
            packages_hit.insert(&info.id);
            packages_expected.extend(info.requirements.iter().map(|(_, id)| id));

            if !self.is_hash_allowed(&info.hash) {
                return false;
            }
        }

        // make sure every dependency is accounted for
        for id in packages_expected {
            if !packages_hit.contains(&id) {
                return false;
            }
        }

        true
    }

    pub fn filter_blocks<'a, 'b: 'a>(
        &'a self,
        game_io: &'a GameIO,
        namespace: PackageNamespace,
        iter: impl Iterator<Item = &'b InstalledBlock> + 'a,
    ) -> impl Iterator<Item = &'b InstalledBlock> + 'a {
        let globals = game_io.resource::<Globals>().unwrap();
        let augment_packages = &globals.augment_packages;

        let blocks: Vec<_> = iter.collect();

        // track count
        let mut block_counts = HashMap::new();

        for block in blocks.iter() {
            let id = &block.package_id;

            block_counts
                .entry((id, block.color))
                .and_modify(|count| *count += 1)
                .or_insert(1);
        }

        blocks.into_iter().filter(move |block| {
            let id = &block.package_id;

            // test ownership
            let placed_count = block_counts.get(&(id, block.color)).unwrap();

            if *placed_count > self.block_count(game_io, id, block.color) {
                return false;
            }

            // test package validity
            let Some(augment) = augment_packages.package_or_fallback(namespace, id) else {
                return false;
            };

            self.validate_package_tree(game_io, augment.package_info.triplet())
        })
    }
}
