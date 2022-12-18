use super::{Folder, ServerInfo};
use crate::packages::*;
use crate::resources::{AssetManager, Globals};
use framework::prelude::GameIO;
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
pub struct GlobalSave {
    pub nickname: String,
    pub selected_character: String,
    pub folders: Vec<Folder>,
    pub selected_folder: usize,
    pub server_list: Vec<ServerInfo>,
}

impl GlobalSave {
    pub const PATH: &str = "save.dat";
    pub const CORRUPTED_PATH: &str = "corrupted_save.dat";

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
                log::error!("failed to load save data: {}", e);
                log::info!("backing up corrupted data to {:?}", Self::CORRUPTED_PATH);

                // crash if we can't back up the corrupted save
                // we never want to accidentally reset a player's save, it should be recoverable
                std::fs::write(Self::CORRUPTED_PATH, bytes).unwrap();

                Self::default()
            }
        }
    }

    pub fn save(&self) {
        use std::fs::File;

        log::info!("saving...");

        let mut file = File::create(Self::PATH).unwrap();

        if let Err(e) = rmp_serde::encode::write_named(&mut file, self) {
            log::error!("failed to save data to {:?}: {}", Self::PATH, e);
        }
    }

    pub fn player_package<'a>(&self, game_io: &'a GameIO<Globals>) -> Option<&'a PlayerPackage> {
        let player_id = &self.selected_character;

        game_io
            .globals()
            .player_packages
            .package_or_fallback(PackageNamespace::Server, player_id)
    }

    pub fn active_folder(&self) -> Option<&Folder> {
        self.folders.get(self.selected_folder)
    }
}

impl Default for GlobalSave {
    fn default() -> Self {
        Self {
            nickname: String::from("Anon"),
            selected_character: String::new(),
            folders: Vec::new(),
            selected_folder: 0,
            server_list: Vec::new(),
        }
    }
}
