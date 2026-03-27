use super::*;
use crate::render::ui::{PackageListing, PackagePreviewData};
use packets::structures::FileHash;
use serde::Deserialize;
use std::sync::Arc;

#[derive(Deserialize, Default)]
#[serde(default)]
struct ResourceMeta {
    category: String,
    name: String,
    long_name: String,
    description: String,
    preview_texture_path: String,
}

#[derive(Default, Clone)]
pub struct ResourcePackage {
    pub package_info: PackageInfo,
    pub name: Arc<str>,
    pub long_name: Arc<str>,
    pub description: Arc<str>,
    pub preview_texture_path: String,
}

impl ResourcePackage {
    pub fn default_package_listing() -> PackageListing {
        let name: Arc<str> = "Default".into();

        PackageListing {
            local: true,
            id: PackageId::default(),
            name: name.clone(),
            long_name: name,
            description: "Default resources for the OS.".into(),
            creator: "Hub OS".into(),
            hash: FileHash::ZERO,
            preview_data: PackagePreviewData::Resource,
            dependencies: Vec::new(),
        }
    }
}

impl Package for ResourcePackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn update_hash(&mut self, hash: FileHash) {
        self.package_info.hash = hash;
    }

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            local: true,
            id: self.package_info.id.clone(),
            name: self.name.clone(),
            long_name: self.long_name.clone(),
            description: self.description.clone(),
            creator: Default::default(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::Resource,
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = Self {
            package_info,
            long_name: Default::default(),
            name: Default::default(),
            description: Default::default(),
            preview_texture_path: Default::default(),
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

        package.name = meta.name.into();
        package.long_name = if meta.long_name.is_empty() {
            package.name.clone()
        } else {
            meta.long_name.into()
        };
        package.description = meta.description.into();
        package.preview_texture_path = meta.preview_texture_path;

        package
    }
}
