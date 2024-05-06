use crate::battle::{BattleProps, PlayerSetup};
use crate::packages::PackageNamespace;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::{SupportingServiceComm, SupportingServiceEvent};
use framework::prelude::*;
use packets::structures::{FileHash, PackageCategory, PackageId};
use serde::{Deserialize, Serialize};
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Clone, Serialize, Deserialize)]
pub struct BattleRecording {
    pub encounter_package_pair: Option<(PackageNamespace, PackageId)>,
    pub data: Option<String>,
    pub seed: u64,
    pub player_setups: Vec<PlayerSetup>,
    pub package_zips: Vec<(PackageCategory, PackageNamespace, Vec<u8>)>,
}

impl BattleRecording {
    pub fn new(props: &BattleProps) -> Self {
        // clear setup buffers
        let mut setups = props.player_setups.clone();

        for setup in &mut setups {
            // we'll put this data back in during battle
            setup.buffer.clear();
        }

        Self {
            encounter_package_pair: if let Some((ns, id)) = &props.encounter_package_pair {
                Some((ns.prefix_recording(), id.clone()))
            } else {
                None
            },
            data: props.data.clone(),
            seed: props.seed,
            player_setups: setups,
            package_zips: Default::default(),
        }
    }

    pub fn save(&mut self, game_io: &GameIO, props: &BattleProps) {
        let service_comm = game_io.resource::<SupportingServiceComm>().unwrap().clone();
        let globals = game_io.resource::<Globals>().unwrap();
        let nickname = globals.global_save.nickname.clone();

        // collect package zips
        if self.package_zips.is_empty() {
            for (info, namespace) in globals.battle_dependencies(game_io, props) {
                let category = info.package_category;
                let namespace = namespace.prefix_recording();
                let hash = &info.hash;

                if let Some(bytes) = globals.assets.virtual_zip_bytes(hash) {
                    self.package_zips.push((category, namespace, bytes));
                } else if *hash != FileHash::ZERO {
                    let path = format!("{}{}.zip", ResourcePaths::MOD_CACHE_FOLDER, hash);
                    let bytes = globals.assets.binary(&path);

                    self.package_zips.push((category, namespace, bytes));
                }

                log::debug!("Saving package: {:?} {:?}", info.id, namespace);
            }
        }

        // get preview path
        let encounter_preview_path = self
            .encounter_package_pair
            .as_ref()
            .and_then(|(ns, id)| {
                let ns = ns.strip_recording();
                globals.encounter_packages.package_or_fallback(ns, id)
            })
            .map(|package| package.preview_texture_path.clone());

        let recording = self.clone();

        log::info!("Starting background thread to save recording");

        std::thread::spawn(move || {
            use std::fs::File;

            // resolve package path
            let elapsed_time = SystemTime::now().duration_since(UNIX_EPOCH).unwrap();
            let unique_id = format!(
                "{}-{}-{nickname}",
                elapsed_time.as_secs(),
                env!("CARGO_PKG_VERSION"),
            );
            let folder_path = format!(
                "{}{}{}-recording/",
                ResourcePaths::game_folder(),
                PackageCategory::Encounter.mod_path(),
                unique_id
            );

            // create parent folder
            let _ = std::fs::create_dir_all(&folder_path);

            // create package file
            let toml_path = folder_path.clone() + "package.toml";
            let toml_data = format!(
                "\
                [package]\n\
                category = \"encounter\"\n\
                id = \"~{unique_id}\"\n\
                name = \"Recorded Battle\"\n\
                description = \"{nickname}'s recorded battle.\"\n\
                preview_texture_path = \"preview.png\"\n\
                recording_path = \"recording.dat\"\n\
                ",
            );

            if let Err(e) = std::fs::write(&toml_path, toml_data) {
                log::error!("Failed to save data to {:?}: {}", toml_path, e);
                return;
            }

            // save preview image
            if let Some(from_path) = encounter_preview_path {
                let to_path = folder_path.clone() + "preview.png";

                if let Err(e) = std::fs::copy(from_path, &to_path) {
                    log::error!("Failed to copy preview texture to {:?}: {}", to_path, e);
                }
            }

            // create recording file
            let dat_path = folder_path.clone() + "recording.dat";
            let mut file = File::create(&dat_path).unwrap();

            if let Err(e) = rmp_serde::encode::write_named(&mut file, &recording) {
                log::error!("Failed to save data to {:?}: {}", dat_path, e);
                return;
            }

            log::info!("Saved recording to {dat_path}");

            service_comm.send(SupportingServiceEvent::LoadPackage {
                category: PackageCategory::Encounter,
                namespace: PackageNamespace::Local,
                path: folder_path,
            })
        });
    }

    pub fn load(assets: &impl AssetManager, path: &str) -> Option<Self> {
        let bytes = assets.binary(path);
        rmp_serde::from_slice(&bytes).ok()
    }

    pub fn load_packages(&self, game_io: &mut GameIO, ignored_package_ids: Vec<PackageId>) {
        let globals = game_io.resource_mut::<Globals>().unwrap();

        // unload old packages
        let loaded_namespaces = globals
            .namespaces()
            .filter(|ns| ns.is_recording())
            .collect::<Vec<_>>();

        for namespace in loaded_namespaces {
            globals.remove_namespace(namespace);
        }

        // load zips
        for (category, namespace, bytes) in &self.package_zips {
            let hash = FileHash::hash(bytes);

            // load zip if it's not already loaded
            let globals = game_io.resource::<Globals>().unwrap();
            let assets = &globals.assets;

            if !assets.contains_virtual_zip(&hash) {
                assets.load_virtual_zip(game_io, hash, bytes.clone());
            }

            // load package through virtual zip
            let globals = game_io.resource_mut::<Globals>().unwrap();
            let package_info = globals.load_virtual_package(*category, *namespace, hash);

            // unloading ignored packages
            let Some(package_info) = package_info else {
                // nothing to unload
                continue;
            };

            if !ignored_package_ids.contains(&package_info.id) {
                // not ignored, we can keep this package
                continue;
            }

            // unload the package
            let id = package_info.id.clone();
            globals.unload_package(*category, *namespace, &id);

            // find the local package to reload in the new namespace
            if let Some(package_info) =
                globals.package_info(*category, PackageNamespace::Local, &id)
            {
                let hash = package_info.hash;
                let zip_path = format!("{}{}.zip", ResourcePaths::MOD_CACHE_FOLDER, hash);

                let globals = game_io.resource::<Globals>().unwrap();
                let bytes = globals.assets.binary(&zip_path);
                globals.assets.load_virtual_zip(game_io, hash, bytes);

                let globals = game_io.resource_mut::<Globals>().unwrap();
                globals.load_virtual_package(*category, *namespace, hash);
            }
        }

        let globals = game_io.resource::<Globals>().unwrap();
        globals.assets.remove_unused_virtual_zips();
    }
}
