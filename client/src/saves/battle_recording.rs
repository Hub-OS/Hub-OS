use crate::battle::{BattleMeta, ExternalEvent, PlayerSetup};
use crate::packages::PackageNamespace;
use crate::render::FrameTime;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::{SupportingServiceComm, SupportingServiceEvent};
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use packets::structures::{FileHash, PackageCategory, PackageId};
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, VecDeque};
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Clone, Default, Serialize, Deserialize)]
pub struct RecordedRollback {
    pub flow_step: usize,
    pub resimulate_time: FrameTime,
}

#[derive(Default, Serialize, Deserialize)]
pub struct RecordedSimulationFlow {
    /// Increments when buffer limits are recorded. (When there's a rewind, or local input is gathered)
    pub current_step: usize,
    /// The step to rewind at, and the frame time to rewind to
    pub rollbacks: VecDeque<RecordedRollback>,
    /// the length of the buffer for each player, repeating for each step
    pub buffer_limits: Vec<u8>,
}

impl RecordedSimulationFlow {
    pub fn get_buffer_limit(&self, player_count: usize, player_index: usize) -> Option<usize> {
        let start_offset = self.current_step * player_count;
        self.buffer_limits
            .get(start_offset + player_index)
            .map(|limit| *limit as usize)
    }
}

#[derive(Serialize, Deserialize)]
pub struct BattleRecording {
    pub encounter_package_pair: Option<(PackageNamespace, PackageId)>,
    pub data: Option<String>,
    pub seed: u64,
    pub player_setups: Vec<PlayerSetup>,
    pub package_map: HashMap<(PackageCategory, FileHash), Vec<u8>>,
    pub required_packages: Vec<(PackageCategory, PackageNamespace, FileHash)>,
    pub external_events: Vec<(FrameTime, ExternalEvent)>,
    pub simulation_flow: Option<RecordedSimulationFlow>,
}

impl BattleRecording {
    pub fn new(meta: &BattleMeta) -> Self {
        // clear setup buffers
        let mut setups = meta.player_setups.clone();

        for setup in &mut setups {
            // we'll put this data back in during battle
            setup.buffer.clear();
        }

        Self {
            encounter_package_pair: if let Some((ns, id)) = &meta.encounter_package_pair {
                Some((ns.prefix_recording(), id.clone()))
            } else {
                None
            },
            data: meta.data.clone(),
            seed: meta.seed,
            player_setups: setups,
            package_map: Default::default(),
            required_packages: Default::default(),
            external_events: Default::default(),
            simulation_flow: if cfg!(feature = "record_simulation_flow") {
                Some(RecordedSimulationFlow::default())
            } else {
                None
            },
        }
    }

    pub fn mark_spectator(&mut self, index: usize) {
        // clear out spectator setup for a smaller recording, and to just reduce memory usage
        let setup = &mut self.player_setups[index];
        setup.package_id = Default::default();
        setup.recipes = Default::default();
        setup.drives = Default::default();
        setup.deck = Default::default();
        setup.blocks = Default::default();
    }

    pub fn save(mut self, game_io: &GameIO, meta: &BattleMeta) {
        let service_comm = game_io.resource::<SupportingServiceComm>().unwrap().clone();
        let globals = game_io.resource::<Globals>().unwrap();
        let nickname = globals.global_save.nickname.clone();

        // collect package zips
        if self.package_map.is_empty() {
            let mut dependency_iter = meta.encounter_dependencies(game_io);
            dependency_iter.extend(meta.player_dependencies(game_io));

            let mod_cache_folder = ResourcePaths::mod_cache_folder();

            for (info, namespace) in dependency_iter {
                let category = info.category;
                let namespace = namespace.prefix_recording();
                let hash = info.hash;

                if hash == FileHash::ZERO {
                    continue;
                }

                self.package_map.entry((category, hash)).or_insert_with(|| {
                    let mut bytes = globals.assets.virtual_zip_bytes(&hash).unwrap_or_else(|| {
                        let path = format!("{}{}.zip", &mod_cache_folder, hash);
                        globals.assets.binary(&path)
                    });

                    if bytes.is_empty() && info.namespace == PackageNamespace::Local {
                        // might be a package that doesn't need to be sent to other players
                        // such as an encounter without any characters defined
                        // try zipping
                        match packets::zip::compress(&info.base_path) {
                            Ok(data) => bytes = data,
                            Err(e) => {
                                log::error!("Failed to zip package {:?}: {e}", info.base_path);
                            }
                        }
                    }

                    bytes
                });

                self.required_packages.push((category, namespace, hash));

                log::debug!("Saving package: {:?} {:?}", info.id, namespace);
            }
        }

        // get preview path
        let encounter_preview = self
            .encounter_package_pair
            .as_ref()
            .and_then(|(ns, id)| {
                let ns = ns.strip_recording();
                globals.encounter_packages.package_or_fallback(ns, id)
            })
            .and_then(|package| {
                let path = &package.preview_texture_path;
                let bytes = globals.assets.binary_silent(path);

                if bytes.is_empty() {
                    return None;
                }

                let file_name = ResourcePaths::file_name(path)?;

                Some((file_name.to_string(), bytes))
            });

        log::info!("Starting background thread to save recording");

        std::thread::spawn(move || {
            use std::fs::File;

            // resolve package path
            let elapsed_time = SystemTime::now().duration_since(UNIX_EPOCH).unwrap();
            let unique_id = format!(
                "{}-{}-{}",
                elapsed_time.as_secs(),
                env!("CARGO_PKG_VERSION"),
                uri_encode(&nickname)
            );
            let folder_path = format!(
                "{}{}{}-recording/",
                ResourcePaths::data_folder(),
                PackageCategory::Encounter.mod_path(),
                unique_id
            );

            // create parent folder
            let _ = std::fs::create_dir_all(&folder_path);

            // create package file
            let preview_image_toml = encounter_preview
                .as_ref()
                .map(|(file_name, _)| format!("preview_texture_path = \"{file_name}\""))
                .unwrap_or_else(|| String::from("# preview_texture_path = \"\""));

            let toml_path = folder_path.clone() + "package.toml";
            let toml_data = format!(
                "\
                [package]\n\
                category = \"encounter\"\n\
                id = \"~{unique_id}\"\n\
                name = \"Recorded Battle\"\n\
                description = \"{}'s recorded battle.\"\n\
                {preview_image_toml}\n\
                recording_path = \"recording.dat\"\n\
                # Add package ids to this list to override recorded packages with installed packages.\n\
                recording_overrides = []\n\
                ",
                nickname.replace('"', "\\\"")
            );

            if let Err(e) = std::fs::write(&toml_path, toml_data) {
                log::error!("Failed to save data to {toml_path:?}: {e}");
                return;
            }

            // save preview image
            if let Some((file_name, bytes)) = encounter_preview {
                let to_path = folder_path.clone() + file_name.as_str();

                if let Err(e) = std::fs::write(&to_path, bytes) {
                    log::error!("Failed to copy preview texture to {to_path:?}: {e}");
                }
            }

            // create recording file
            let dat_path = folder_path.clone() + "recording.dat";
            let mut file = File::create(&dat_path).unwrap();

            if let Err(e) = rmp_serde::encode::write_named(&mut file, &self) {
                log::error!("Failed to save data to {dat_path:?}: {e}");
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
        rmp_serde::from_slice(&bytes)
            .inspect_err(|e| log::error!("Failed to read recording: {e:?}"))
            .ok()
    }

    pub fn load_packages(&self, game_io: &mut GameIO, ignored_package_ids: Vec<PackageId>) {
        let globals = game_io.resource_mut::<Globals>().unwrap();
        let mod_cache_folder = ResourcePaths::mod_cache_folder();

        // unload old packages
        let loaded_namespaces = globals
            .namespaces()
            .filter(|ns| ns.is_recording())
            .collect::<Vec<_>>();

        for namespace in loaded_namespaces {
            globals.remove_namespace(namespace);
        }

        // load zips
        for &(category, namespace, hash) in &self.required_packages {
            // load zip if it's not already loaded
            let globals = game_io.resource::<Globals>().unwrap();
            let assets = &globals.assets;

            if !assets.contains_virtual_zip(&hash)
                && let Some(bytes) = self.package_map.get(&(category, hash))
            {
                assets.load_virtual_zip(game_io, hash, bytes.clone());
            }

            // load package through virtual zip
            let globals = game_io.resource_mut::<Globals>().unwrap();
            let package_info = globals.load_virtual_package(category, namespace, hash);

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
            globals.unload_package(category, namespace, &id);

            // find the local package to reload in the new namespace
            if let Some(package_info) = globals.package_info(category, PackageNamespace::Local, &id)
            {
                let hash = package_info.hash;
                let zip_path = format!("{}{}.zip", &mod_cache_folder, hash);

                let globals = game_io.resource::<Globals>().unwrap();
                let bytes = globals.assets.binary(&zip_path);
                globals.assets.load_virtual_zip(game_io, hash, bytes);

                let globals = game_io.resource_mut::<Globals>().unwrap();
                globals.load_virtual_package(category, namespace, hash);
            }
        }

        let globals = game_io.resource::<Globals>().unwrap();
        globals.assets.remove_unused_virtual_zips();
    }
}
