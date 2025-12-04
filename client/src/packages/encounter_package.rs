use super::*;
use crate::render::ui::{PackageListing, PackagePreviewData};
use serde::Deserialize;
use std::sync::Arc;

#[derive(Deserialize, Default)]
#[serde(default)]
struct EncounterMeta {
    category: String,
    name: String,
    long_name: String,
    description: String,
    preview_texture_path: String,
    recording_path: Option<String>,
    recording_overrides: Vec<PackageId>,
}

#[derive(Default, Clone)]
pub struct EncounterPackage {
    pub package_info: PackageInfo,
    pub name: Arc<str>,
    pub long_name: Arc<str>,
    pub description: Arc<str>,
    pub preview_texture_path: String,
    pub recording_path: Option<String>,
    pub recording_overrides: Vec<PackageId>,
}

impl Package for EncounterPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
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
            preview_data: PackagePreviewData::Encounter {
                recording: self.recording_path.is_some(),
            },
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = EncounterPackage {
            package_info,
            ..Default::default()
        };

        let meta: EncounterMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "encounter" {
            log::error!(
                "Missing `category = \"encounter\"` in {:?}",
                package.package_info.toml_path
            );
        }

        let base_path = &package.package_info.base_path;

        package.name = meta.name.into();
        package.long_name = if meta.long_name.is_empty() {
            package.name.clone()
        } else {
            meta.long_name.into()
        };
        package.description = meta.description.into();
        package.preview_texture_path = base_path.clone() + &meta.preview_texture_path;
        package.recording_path = meta.recording_path.map(|path| base_path.clone() + &path);
        package.recording_overrides = meta.recording_overrides;

        if package.recording_path.is_some() {
            package.package_info.shareable = false;
        }

        package
    }
}
