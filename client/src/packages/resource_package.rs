use super::*;
use crate::render::ui::{PackageListing, PackagePreviewData};
use crate::resources::LocalAssetManager;
use framework::prelude::GameIO;
use packets::structures::FileHash;
use serde::Deserialize;
use walkdir::WalkDir;

#[derive(Deserialize, Default)]
#[serde(default)]
struct ResourceMeta {
    category: String,
    name: String,
    description: String,
}

#[derive(Default, Clone)]
pub struct ResourcePackage {
    pub package_info: PackageInfo,
    name: String,
    description: String,
}

impl ResourcePackage {
    pub fn apply(&self, game_io: &GameIO, assets: &LocalAssetManager) {
        let base_path = &self.package_info.base_path;
        let resources_path = base_path.clone() + "resources";
        let path_skip = resources_path.len() - "resources".len();

        for entry in WalkDir::new(resources_path).into_iter().flatten() {
            let Ok(metadata) = entry.metadata() else {
                continue;
            };

            if metadata.is_dir() {
                continue;
            }

            let file_path = &entry.path().to_string_lossy()[..];
            let resource_path = &file_path[path_skip..];

            assets.override_cache(game_io, resource_path, file_path);
        }
    }

    pub fn default_package_listing() -> PackageListing {
        PackageListing {
            id: PackageId::default(),
            name: String::from("Default"),
            description: String::from("Default resources for the OS."),
            creator: String::from("Hub OS"),
            hash: FileHash::ZERO,
            preview_data: PackagePreviewData::Resources,
            dependencies: Vec::new(),
        }
    }
}

impl Package for ResourcePackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
    }

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            id: self.package_info.id.clone(),
            name: self.name.clone(),
            description: self.description.clone(),
            creator: String::new(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::Resources,
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = Self {
            package_info,
            name: String::new(),
            description: String::new(),
        };

        let meta: ResourceMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "resource" {
            log::error!(
                "Missing `category = \"resource\"` in {:?}",
                package.package_info.toml_path
            );
        }

        package.name = meta.name;
        package.description = meta.description;

        package
    }
}

impl PackageManager<ResourcePackage> {
    pub fn apply(
        &self,
        game_io: &GameIO,
        resource_package_order: &[(PackageId, bool)],
        assets: &LocalAssetManager,
    ) {
        for (id, enabled) in resource_package_order.iter().rev() {
            if !enabled {
                continue;
            }

            let Some(package) = self.package(PackageNamespace::Local, id) else {
                continue;
            };

            package.apply(game_io, assets);
        }
    }
}
