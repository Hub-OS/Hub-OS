use super::*;
use crate::render::ui::{PackageListing, PackagePreviewData};
use packets::structures::FileHash;
use serde::Deserialize;
use std::sync::Arc;

#[derive(Default, Clone)]
pub enum LibrarySubCategory {
    #[default]
    None,
    Pack,
}

#[derive(Deserialize, Default)]
#[serde(default)]
struct LibraryMeta {
    category: String,
    name: String,
    long_name: String,
    description: String,
    preview_texture_path: String,
    tags: Vec<String>,
}

#[derive(Default, Clone)]
pub struct LibraryPackage {
    pub package_info: PackageInfo,
    pub name: Arc<str>,
    pub long_name: Arc<str>,
    pub description: Arc<str>,
    pub preview_texture_path: String,
    pub sub_category: LibrarySubCategory,
}

impl Package for LibraryPackage {
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
            preview_data: match self.sub_category {
                LibrarySubCategory::None => PackagePreviewData::Library,
                LibrarySubCategory::Pack => PackagePreviewData::Pack,
            },
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = Self {
            package_info,
            name: Default::default(),
            long_name: Default::default(),
            description: Default::default(),
            preview_texture_path: Default::default(),
            sub_category: Default::default(),
        };

        let meta: LibraryMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        package.package_info.tags = meta.tags.into_iter().map(|s| s.into()).collect();

        if meta.category != "library" && meta.category != "pack" {
            log::error!(
                "Missing `category = \"library\"` in {:?}",
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

        if meta.category == "pack" {
            package.sub_category = LibrarySubCategory::Pack;
        }

        package
    }
}
