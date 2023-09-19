use crate::battle::{BattleProps, PlayerSetup};
use crate::packages::PackageNamespace;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::*;
use packets::structures::{FileHash, PackageCategory, PackageId};
use serde::{Deserialize, Serialize};
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Serialize, Deserialize)]
pub struct BattleRecording {
    pub encounter_package_pair: Option<(PackageNamespace, PackageId)>,
    pub data: Option<String>,
    pub seed: u64,
    pub player_setups: Vec<PlayerSetup>,
    pub package_zips: Vec<(PackageCategory, PackageNamespace, Vec<u8>)>,
}

impl BattleRecording {
    fn normalize_namespace(
        namespace: PackageNamespace,
        local_index: Option<usize>,
    ) -> PackageNamespace {
        match namespace {
            PackageNamespace::Netplay(i) => PackageNamespace::Netplay(i),
            PackageNamespace::RecordingServer | PackageNamespace::Server => {
                PackageNamespace::RecordingServer
            }
            PackageNamespace::Local => {
                if let Some(i) = local_index {
                    PackageNamespace::Netplay(i)
                } else {
                    log::error!(
                        "Unabled to handle Local namespace in BattleRecording::normalize_namespace()"
                    );

                    PackageNamespace::RecordingServer
                }
            }
        }
    }

    pub fn new(game_io: &GameIO, props: &BattleProps) -> Self {
        let local_index = (props.player_setups.iter())
            .find(|setup| setup.local)
            .map(|setup| setup.index);

        // collect package zips
        let mut package_zips = Vec::new();
        let globals = game_io.resource::<Globals>().unwrap();

        for (info, namespace) in globals.battle_dependencies(game_io, props) {
            let category = info.package_category;
            let namespace = Self::normalize_namespace(namespace, local_index);
            let hash = &info.hash;

            if let Some(bytes) = globals.assets.virtual_zip_bytes(hash) {
                package_zips.push((category, namespace, bytes));
            } else if *hash != FileHash::ZERO {
                let path = format!("{}{}.zip", ResourcePaths::MOD_CACHE_FOLDER, hash);
                let bytes = globals.assets.binary(&path);

                package_zips.push((category, namespace, bytes));
            }
        }

        // clear setup buffers
        let mut setups = props.player_setups.clone();

        for setup in &mut setups {
            // we'll put this data back in during battle
            setup.buffer.clear();
            // fix namespaces
            setup.package_pair.0 = Self::normalize_namespace(setup.package_pair.0, local_index);
        }

        Self {
            encounter_package_pair: if let Some((ns, id)) = &props.encounter_package_pair {
                Some((Self::normalize_namespace(*ns, local_index), id.clone()))
            } else {
                None
            },
            data: props.data.clone(),
            seed: props.seed,
            player_setups: setups,
            package_zips,
        }
    }

    pub fn save(&self, game_io: &mut GameIO) {
        use std::fs::File;

        // resolve package path
        let elapsed_time = SystemTime::now().duration_since(UNIX_EPOCH).unwrap();
        let unique_id = format!("{}-{}", elapsed_time.as_secs(), env!("CARGO_PKG_VERSION"));
        let folder_path = format!(
            "{}{}{}-recording/",
            ResourcePaths::game_folder(),
            PackageCategory::Encounter.path(),
            unique_id
        );

        // create parent folder
        let _ = std::fs::create_dir_all(&folder_path);

        // create package file
        let toml_path = folder_path.clone() + "package.toml";
        let globals = game_io.resource_mut::<Globals>().unwrap();
        let nickname = &globals.global_save.nickname;
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
        }

        // create recording file
        let dat_path = folder_path.clone() + "recording.dat";
        let mut file = File::create(&dat_path).unwrap();

        if let Err(e) = rmp_serde::encode::write_named(&mut file, self) {
            log::error!("Failed to save data to {:?}: {}", dat_path, e);
            return;
        }

        // save image
        let encounter_package = self
            .encounter_package_pair
            .as_ref()
            .and_then(|(ns, id)| globals.encounter_packages.package_or_override(*ns, id));

        if let Some(package) = encounter_package {
            let to_path = folder_path.clone() + "preview.png";
            let from_path = &package.preview_texture_path;

            if let Err(e) = std::fs::copy(from_path, &to_path) {
                log::error!("Failed to copy preview texture to {:?}: {}", to_path, e);
            }
        }

        log::info!("Saved recording to {dat_path}");

        // load the newly created package
        globals.encounter_packages.load_package(
            &globals.assets,
            PackageNamespace::Local,
            &folder_path,
        );
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

            if namespace.has_override(PackageNamespace::Local) {
                // package can be overridden by local packages
                // no need to load a package with the same namespace
                continue;
            }

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
