use super::{DeckRestrictions, Globals};
use crate::{packages::PackageNamespace, render::FrameTime};
use framework::prelude::GameIO;
use packets::structures::{FileHash, InstalledBlock, PackageCategory, PackageId};
use std::collections::HashSet;

pub struct Restrictions {
    pub package_whitelist: HashSet<FileHash>,
    pub package_blacklist: HashSet<FileHash>,
    pub default_deck_restrictions: DeckRestrictions,
    // battle restrictions, used to generate initial BattleConfig
    // todo: not in use
    pub status_durations: [FrameTime; 3],
    pub intangibility_duration: FrameTime,
    pub super_effective_multiplier: f32,
    pub turn_limit: Option<u32>,
}

impl Default for Restrictions {
    fn default() -> Self {
        Self {
            package_whitelist: HashSet::new(),
            package_blacklist: HashSet::new(),
            default_deck_restrictions: DeckRestrictions::default(),
            status_durations: [90, 120, 150],
            intangibility_duration: 120,
            super_effective_multiplier: 2.0,
            turn_limit: None,
        }
    }
}

impl Restrictions {
    fn is_hash_allowed(&self, hash: &FileHash) -> bool {
        (self.package_whitelist.is_empty() || self.package_whitelist.contains(hash))
            && (self.package_blacklist.is_empty() || !self.package_blacklist.contains(hash))
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

    pub fn filter_blocks<'a, 'b>(
        &'a self,
        game_io: &'a GameIO,
        namespace: PackageNamespace,
        iter: impl Iterator<Item = &'b InstalledBlock> + 'a,
    ) -> impl Iterator<Item = &'b InstalledBlock> + 'a {
        let globals = game_io.resource::<Globals>().unwrap();
        let augment_packages = &globals.augment_packages;

        iter.filter(move |block| {
            let Some(augment) = augment_packages.package_or_fallback(namespace, &block.package_id) else {
                return false;
            };

            self.validate_package_tree(game_io, augment.package_info.triplet())
        })
    }
}
