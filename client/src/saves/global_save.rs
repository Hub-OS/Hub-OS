use super::{BlockGrid, Deck, InstalledBlock, ServerInfo};
use crate::packages::*;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::GameIO;
use packets::structures::{InstalledSwitchDrive, MemoryCell, Uuid};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

const FIRST_SERVER_NAME: &str = "The Index";
const FIRST_SERVER_ADDRESS: &str = "servers.hubos.dev";

#[derive(Serialize, Deserialize)]
#[serde(default)]
pub struct GlobalSave {
    pub nickname: String,
    #[serde(default = "GlobalSave::current_time")]
    pub nickname_time: u64,
    pub server_list: Vec<ServerInfo>,
    pub decks: Vec<Deck>,
    pub selected_deck: usize,
    #[serde(default = "GlobalSave::current_time")]
    pub selected_deck_time: u64,
    pub selected_character: PackageId,
    #[serde(default = "GlobalSave::current_time")]
    pub selected_character_time: u64,
    pub character_update_times: HashMap<PackageId, u64>,
    pub installed_blocks: HashMap<PackageId, Vec<InstalledBlock>>,
    pub installed_drive_parts: HashMap<PackageId, Vec<InstalledSwitchDrive>>,
    pub memories: HashMap<PackageId, HashMap<String, MemoryCell>>,
    pub resource_package_order: Vec<(PackageId, bool)>,
    #[serde(default = "GlobalSave::current_time")]
    pub resource_order_time: u64,
    pub virtual_controller_visible: bool,
}

impl GlobalSave {
    fn path() -> String {
        ResourcePaths::data_folder_absolute("save.dat")
    }

    pub fn current_time() -> u64 {
        std::time::UNIX_EPOCH
            .elapsed()
            .unwrap_or_default()
            .as_secs()
    }

    pub fn new() -> Self {
        Self::default()
    }

    pub fn load(assets: &impl AssetManager) -> Self {
        let bytes = assets.binary_silent(&Self::path());

        if bytes.is_empty() {
            // no save data
            // this is an extra check to prevent errors from appearing
            return Self::default();
        }

        let mut save: Self = match rmp_serde::from_slice(&bytes) {
            Ok(save) => save,
            Err(e) => {
                let corrupted_path = ResourcePaths::data_folder_absolute("corrupted_save.dat");
                log::error!("Failed to load save data: {e}");
                log::info!("Backing up corrupted data to {corrupted_path:?}");

                // crash if we can't back up the corrupted save
                // we never want to accidentally reset a player's save, it should be recoverable
                std::fs::write(corrupted_path, bytes).unwrap();

                return Self::default();
            }
        };

        // backwards compat for saves from before data sync existed
        if save.character_update_times.is_empty() {
            let time = GlobalSave::current_time();
            let mut updated = false;

            for key in save.installed_blocks.keys() {
                save.character_update_times.insert(key.clone(), time);
                updated = true;
            }

            for key in save.installed_drive_parts.keys() {
                save.character_update_times.insert(key.clone(), time);
                updated = true;
            }

            // make sure the pre-installed server has a nil uuid
            if save.server_list.iter().all(|info| !info.uuid.is_nil()) {
                for server_info in &mut save.server_list {
                    if server_info.address == FIRST_SERVER_ADDRESS {
                        if !server_info.uuid.is_nil() {
                            server_info.uuid = Uuid::nil();
                            server_info.update_time = time;
                            updated = true;
                        }

                        break;
                    }
                }
            }

            if updated {
                save.save();
            }
        }

        save
    }

    pub fn save(&self) {
        use std::fs::File;

        log::info!("Saving...");

        let path = Self::path();
        let mut file = File::create(&path).unwrap();

        if let Err(e) = rmp_serde::encode::write_named(&mut file, self) {
            log::error!("Failed to save data to {path:?}: {e}");
        }
    }

    pub fn sync(&mut self, mut other: GlobalSave) {
        // sync nickname
        if other.nickname_time > self.nickname_time {
            self.nickname = other.nickname;
            self.nickname_time = other.nickname_time;
        }

        // sync server list
        let mut prev_server = None;

        for other_info in other.server_list.into_iter() {
            let insert_after = prev_server;
            prev_server = Some(other_info.uuid);

            let mut info_iter = self.server_list.iter_mut();
            let Some(info) = info_iter.find(|info| info.uuid == other_info.uuid) else {
                if let Some(uuid) = insert_after {
                    let mut info_iter = self.server_list.iter();
                    let i = info_iter
                        .position(|info| info.uuid == uuid)
                        .unwrap_or_default();

                    self.server_list.insert(i + 1, other_info);
                } else {
                    self.server_list.insert(0, other_info);
                }

                continue;
            };

            // sync server info
            if other_info.update_time > info.update_time {
                *info = other_info;
            }
        }

        // sync decks
        let should_merge_selected_deck = other.selected_deck_time > self.selected_deck_time;
        let mut other_selected_deck_uuid = None;

        for (other_i, other_deck) in other.decks.into_iter().enumerate() {
            if other_i == other.selected_deck {
                other_selected_deck_uuid = Some(other_deck.uuid);
            }

            let mut deck_iter = self.decks.iter_mut();
            let Some(deck) = deck_iter.find(|deck| deck.uuid == other_deck.uuid) else {
                // append deck
                self.decks.push(other_deck);
                continue;
            };

            // sync deck data
            if other_deck.update_time > deck.update_time {
                *deck = other_deck;
            }
        }

        // sync selected deck
        if should_merge_selected_deck {
            let other_selected_index = other_selected_deck_uuid
                .and_then(|uuid| self.decks.iter().position(|d| d.uuid == uuid));

            if let Some(i) = other_selected_index {
                self.selected_deck = i;
            }

            self.selected_deck_time = other.selected_deck_time
        }

        // sync selected character
        if other.selected_character_time > self.selected_character_time {
            self.selected_character = other.selected_character;
            self.selected_character_time = other.selected_character_time;
        }

        // sync augments
        for (id, other_time) in other.character_update_times {
            let time = self.character_update_times.get(&id);

            if time.is_none_or(|t| other_time > *t) {
                if let Some(blocks) = other.installed_blocks.remove(&id) {
                    self.installed_blocks.insert(id.clone(), blocks);
                }

                if let Some(parts) = other.installed_drive_parts.remove(&id) {
                    self.installed_drive_parts.insert(id.clone(), parts);
                }

                self.character_update_times.insert(id, other_time);
            }
        }

        // sync resource order
        if other.resource_order_time > self.resource_order_time {
            self.resource_package_order = other.resource_package_order;
            self.resource_order_time = other.resource_order_time;
        }
    }

    pub fn player_package<'a>(&self, game_io: &'a GameIO) -> Option<&'a PlayerPackage> {
        let player_id = &self.selected_character;

        let globals = Globals::from_resources(game_io);

        globals
            .player_packages
            .package(PackageNamespace::Local, player_id)
    }

    pub fn active_deck(&self) -> Option<&Deck> {
        self.decks.get(self.selected_deck)
    }

    pub fn active_blocks(&self) -> &[InstalledBlock] {
        self.installed_blocks
            .get(&self.selected_character)
            .map(|blocks| blocks.as_slice())
            .unwrap_or(&[])
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

        let globals = Globals::from_resources(game_io);
        let global_save = &globals.global_save;
        let restrictions = &globals.restrictions;

        // blocks
        let blocks: Vec<_> = restrictions
            .filter_blocks(game_io, NAMESPACE, global_save.active_blocks().iter())
            .cloned()
            .collect();

        let block_grid = BlockGrid::new(NAMESPACE).with_blocks(game_io, blocks);

        let blocks_iter = block_grid.augments(game_io);

        // drives
        let drives = restrictions
            .filter_drives(game_io, NAMESPACE, global_save.active_drive_parts().iter())
            .cloned();

        let drives_iter = drives
            .flat_map(|drive| {
                globals
                    .augment_packages
                    .package(NAMESPACE, &drive.package_id)
            })
            .map(|augment| (augment, 1));

        // chain augments
        blocks_iter.chain(drives_iter)
    }

    pub fn include_new_resources(&mut self, packages: &PackageManager<ResourcePackage>) -> bool {
        // update global save with missing entries
        let package_id_iter = packages
            .package_ids(PackageNamespace::BuiltIn)
            .chain(packages.package_ids(PackageNamespace::Local));

        let mut updated_order = false;

        for id in package_id_iter {
            if !self
                .resource_package_order
                .iter()
                .any(|(saved_id, _)| saved_id == id)
            {
                self.resource_package_order.push((id.clone(), true));
                updated_order = true;
            }
        }

        updated_order
    }

    pub fn update_memories(
        &mut self,
        package_id: &PackageId,
        memories: Option<HashMap<String, MemoryCell>>,
    ) {
        if let Some(memories) = memories.filter(|m| !m.is_empty()) {
            match self.memories.entry(package_id.clone()) {
                std::collections::hash_map::Entry::Occupied(mut occupied_entry) => {
                    if *occupied_entry.get() != memories {
                        occupied_entry.insert(memories);
                        self.save();
                    }
                }
                std::collections::hash_map::Entry::Vacant(vacant_entry) => {
                    vacant_entry.insert(memories);
                    self.save();
                }
            }
        } else {
            let removed_memory = self.memories.remove(package_id).is_some();

            if removed_memory {
                self.save();
            }
        }
    }

    pub fn update_package_id(
        &mut self,
        category: PackageCategory,
        old_id: &PackageId,
        new_id: &PackageId,
    ) {
        if old_id == new_id {
            return;
        }

        log::info!("Updating save for updated package id: {old_id} -> {new_id}");

        match category {
            PackageCategory::Augment => {
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
            }
            PackageCategory::Card => {
                // update decks
                for deck in &mut self.decks {
                    for card in &mut deck.cards {
                        if card.package_id == *old_id {
                            card.package_id = new_id.clone();
                        }
                    }
                }
            }
            PackageCategory::Player => {
                // update selected character
                if self.selected_character == *old_id {
                    self.selected_character = new_id.clone();
                }

                if let Some(time) = self.character_update_times.remove(old_id) {
                    self.character_update_times.insert(new_id.clone(), time);
                }

                // update memories
                if let Some(memory) = self.memories.remove(old_id) {
                    self.memories.insert(new_id.clone(), memory);
                }
            }
            PackageCategory::Resource => {
                // update resources
                for (id, _) in &mut self.resource_package_order {
                    if id == old_id {
                        *id = new_id.clone();
                    }
                }
            }
            _ => {}
        }
    }
}

impl Default for GlobalSave {
    fn default() -> Self {
        Self {
            nickname: String::from("Anon"),
            nickname_time: 0,
            server_list: vec![ServerInfo {
                name: FIRST_SERVER_NAME.to_string(),
                address: FIRST_SERVER_ADDRESS.to_string(),
                uuid: Uuid::nil(),
                update_time: 0,
            }],
            decks: Vec::new(),
            selected_deck: 0,
            selected_deck_time: 0,
            selected_character: PackageId::new_blank(),
            selected_character_time: 0,
            character_update_times: HashMap::new(),
            installed_blocks: HashMap::new(),
            installed_drive_parts: HashMap::new(),
            memories: HashMap::new(),
            resource_package_order: Vec::new(),
            resource_order_time: 0,
            virtual_controller_visible: true,
        }
    }
}
