use super::{BlockGrid, Deck, InstalledBlock, ServerInfo};
use crate::packages::*;
use crate::resources::{AssetManager, Globals};
use framework::prelude::GameIO;
use packets::structures::InstalledSwitchDrive;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Serialize, Deserialize)]
#[serde(default)]
pub struct GlobalSave {
    pub nickname: String,
    pub selected_character: PackageId,
    pub decks: Vec<Deck>,
    pub selected_deck: usize,
    pub server_list: Vec<ServerInfo>,
    pub installed_blocks: HashMap<PackageId, Vec<InstalledBlock>>,
    pub installed_drive_parts: HashMap<PackageId, Vec<InstalledSwitchDrive>>,
    pub resource_package_order: Vec<(PackageId, bool)>,
}

impl GlobalSave {
    pub const PATH: &'static str = "save.dat";
    pub const CORRUPTED_PATH: &'static str = "corrupted_save.dat";

    pub fn new() -> Self {
        Self::default()
    }

    pub fn load(assets: &impl AssetManager) -> Self {
        let bytes = assets.binary(Self::PATH);

        if bytes.is_empty() {
            // no save data
            // this is an extra check to prevent errors from appearing
            return Self::default();
        }

        match rmp_serde::from_slice(&bytes) {
            Ok(save) => save,
            Err(e) => {
                log::error!("Failed to load save data: {}", e);
                log::info!("Backing up corrupted data to {:?}", Self::CORRUPTED_PATH);

                // crash if we can't back up the corrupted save
                // we never want to accidentally reset a player's save, it should be recoverable
                std::fs::write(Self::CORRUPTED_PATH, bytes).unwrap();

                Self::default()
            }
        }
    }

    pub fn save(&self) {
        use std::fs::File;

        log::info!("Saving...");

        let mut file = File::create(Self::PATH).unwrap();

        if let Err(e) = rmp_serde::encode::write_named(&mut file, self) {
            log::error!("Failed to save data to {:?}: {}", Self::PATH, e);
        }
    }

    pub fn player_package<'a>(&self, game_io: &'a GameIO) -> Option<&'a PlayerPackage> {
        let player_id = &self.selected_character;

        let globals = game_io.resource::<Globals>().unwrap();

        globals
            .player_packages
            .package_or_override(PackageNamespace::Local, player_id)
    }

    pub fn active_deck(&self) -> Option<&Deck> {
        self.decks.get(self.selected_deck)
    }

    pub fn active_blocks(&self) -> Option<&Vec<InstalledBlock>> {
        self.installed_blocks.get(&self.selected_character)
    }

    pub fn active_drive_parts(&self) -> &[InstalledSwitchDrive] {
        self.installed_drive_parts
            .get(&self.selected_character)
            .map(|parts| parts.as_slice())
            .unwrap_or(&[])
    }

    pub fn valid_augments<'a>(
        &self,
        game_io: &'a GameIO,
    ) -> impl Iterator<Item = (&'a AugmentPackage, usize)> + 'a {
        const NAMESPACE: PackageNamespace = PackageNamespace::Local;

        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;
        let restrictions = &globals.restrictions;

        let blocks: Vec<_> = if let Some(blocks) = global_save.active_blocks() {
            restrictions
                .filter_blocks(game_io, NAMESPACE, blocks.iter())
                .cloned()
                .collect()
        } else {
            Vec::new()
        };

        let block_grid = BlockGrid::new(NAMESPACE).with_blocks(game_io, blocks);

        block_grid.augments(game_io)
    }

    pub fn update_package_id(&mut self, old_id: &PackageId, new_id: &PackageId) {
        if old_id == new_id {
            return;
        }

        // update selected character
        if self.selected_character == *old_id {
            self.selected_character = new_id.clone();
        }

        // update decks
        for deck in &mut self.decks {
            for card in &mut deck.cards {
                if card.package_id == *old_id {
                    card.package_id = new_id.clone();
                }
            }
        }

        // update blocks
        if let Some(blocks) = self.installed_blocks.remove(old_id) {
            self.installed_blocks.insert(new_id.clone(), blocks);
        }

        for blocks in &mut self.installed_blocks.values_mut() {
            for block in blocks {
                if block.package_id == *old_id {
                    block.package_id = new_id.clone();
                }
            }
        }

        // update switch drive parts
        if let Some(parts) = self.installed_drive_parts.remove(old_id) {
            self.installed_drive_parts.insert(new_id.clone(), parts);
        }

        for parts in &mut self.installed_drive_parts.values_mut() {
            for part in parts {
                if part.package_id == *old_id {
                    part.package_id = new_id.clone();
                }
            }
        }

        // update resources
        for (id, _) in &mut self.resource_package_order {
            if id == old_id {
                *id = new_id.clone();
            }
        }
    }
}

impl Default for GlobalSave {
    fn default() -> Self {
        Self {
            nickname: String::from("Anon"),
            selected_character: PackageId::new_blank(),
            decks: Vec::new(),
            selected_deck: 0,
            server_list: Vec::new(),
            installed_blocks: HashMap::new(),
            installed_drive_parts: HashMap::new(),
            resource_package_order: Vec::new(),
        }
    }
}
