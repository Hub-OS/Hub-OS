use super::{DeckRestrictions, Globals};
use crate::packages::PackageInfo;
use crate::{packages::PackageNamespace, saves::Card};
use framework::prelude::GameIO;
use packets::structures::{BlockColor, FileHash, InstalledBlock, PackageCategory, PackageId};
use std::borrow::Cow;
use std::collections::{HashMap, HashSet};

#[derive(Default)]
pub struct Restrictions {
    hash_whitelist: HashSet<FileHash>,
    id_whitelist: HashSet<PackageId>,
    id_blacklist: HashSet<PackageId>,
    base_deck_restrictions: DeckRestrictions,
    owned_cards: HashMap<Card, usize>,
    owned_blocks: HashMap<(Cow<'static, PackageId>, BlockColor), usize>,
    owned_players: HashSet<PackageId>,
}

impl Restrictions {
    pub fn load_restrictions_toml(&mut self, text: String) {
        let mut root_table: toml::Table = text.parse().unwrap_or_else(|err| {
            log::error!("Failed to parse restrictions: {err}");
            toml::Table::default()
        });

        // [deck]
        self.base_deck_restrictions = root_table
            .remove("deck")
            .and_then(|value| match toml::Value::try_into(value) {
                Ok(restrictions) => Some(restrictions),
                Err(err) => {
                    log::error!("Failed to parse deck restrictions: {err}");
                    None
                }
            })
            .unwrap_or_default();

        // [packages]
        let mut packages_table = root_table.remove("packages").and_then(|value| match value {
            toml::Value::Table(table) => Some(table),
            _ => None,
        });

        let file_hashset_mapper = |mut value: toml::Value| {
            value.as_array_mut().map(|array| {
                let values = std::mem::take(array);

                values
                    .into_iter()
                    .flat_map(|value| value.as_str().and_then(FileHash::from_hex))
                    .collect::<HashSet<FileHash>>()
            })
        };

        let package_id_mapper = |mut value: toml::Value| {
            value.as_array_mut().map(|array| {
                let values = std::mem::take(array);

                values
                    .into_iter()
                    .flat_map(|value| value.as_str().map(PackageId::from))
                    .collect::<HashSet<PackageId>>()
            })
        };

        // packages.hash_whitelist
        self.hash_whitelist = packages_table
            .as_mut()
            .and_then(|packages_table| packages_table.remove("hash_whitelist"))
            .and_then(file_hashset_mapper)
            .unwrap_or_default();

        // packages.id_whitelist
        self.id_whitelist = packages_table
            .as_mut()
            .and_then(|packages_table| packages_table.remove("id_whitelist"))
            .and_then(package_id_mapper)
            .unwrap_or_default();

        // packages.id_blacklist
        self.id_blacklist = packages_table
            .and_then(|mut packages_table| packages_table.remove("id_blacklist"))
            .and_then(package_id_mapper)
            .unwrap_or_default();
    }

    fn is_package_allowed(&self, info: &PackageInfo) -> bool {
        (self.id_whitelist.is_empty() || self.id_whitelist.contains(&info.id))
            && (self.id_blacklist.is_empty() || !self.id_blacklist.contains(&info.id))
    }

    pub fn base_deck_restrictions(&self) -> DeckRestrictions {
        self.base_deck_restrictions.clone()
    }

    pub fn card_ownership_enabled(&self) -> bool {
        !self.owned_cards.is_empty()
    }

    pub fn card_iter(&self) -> impl Iterator<Item = (&Card, usize)> {
        self.owned_cards.iter().map(|(card, count)| (card, *count))
    }

    pub fn card_count(&self, game_io: &GameIO, card: &Card) -> usize {
        if self.owned_cards.is_empty() {
            // use package info
            let globals = game_io.resource::<Globals>().unwrap();
            let card_packages = &globals.card_packages;

            card_packages
                .package_or_fallback(PackageNamespace::Local, &card.package_id)
                .map(|package| {
                    if package.default_codes.contains(&card.code) {
                        package.limit
                    } else {
                        0
                    }
                })
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

            let valid_color = augment_packages
                .package_or_fallback(PackageNamespace::Local, id)
                .is_some_and(|package| {
                    let mut color_iter = package.block_colors.iter();
                    color_iter.any(|&listed_color| listed_color == color)
                });

            if valid_color {
                9
            } else {
                0
            }
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

    pub fn add_card(&mut self, card: Card, count: isize) {
        self.owned_cards
            .entry(card)
            .and_modify(|existing_count| {
                *existing_count = (*existing_count as isize + count).max(0) as usize;
            })
            .or_insert(if count > 0 { count as usize } else { 0 });
    }

    pub fn add_block(&mut self, package_id: PackageId, color: BlockColor, count: isize) {
        let key = (Cow::Owned(package_id), color);

        self.owned_blocks
            .entry(key)
            .and_modify(|existing_count| {
                *existing_count = (*existing_count as isize + count).max(0) as usize;
            })
            .or_insert(if count > 0 { count as usize } else { 0 });
    }

    pub fn enable_player_character(&mut self, package_id: PackageId, enabled: bool) {
        if enabled {
            self.owned_players.insert(package_id);
        } else {
            self.owned_players.remove(&package_id);
        }
    }

    /// Returns true if the package and its dependencies are valid
    pub fn validate_package_tree(
        &self,
        game_io: &GameIO,
        triplet: (PackageCategory, PackageNamespace, PackageId),
    ) -> bool {
        let package_id = triplet.2.clone();

        let globals = game_io.resource::<Globals>().unwrap();
        let deps = globals.package_dependency_iter(std::iter::once(triplet));

        // test hashes while tracking dependencies
        let mut packages_hit = HashSet::new();
        let mut packages_expected = HashSet::new();

        for (info, _) in deps.iter() {
            packages_hit.insert(&info.id);
            packages_expected.extend(info.requirements.iter().map(|(_, id)| id));

            if !self.is_package_allowed(info) {
                return false;
            }
        }

        // make sure every dependency is accounted for
        for id in packages_expected {
            if !packages_hit.contains(&id) {
                log::warn!("Missing dependency {id} for {package_id}");
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
