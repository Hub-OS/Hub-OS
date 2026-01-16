use super::*;
use crate::render::FrameTime;
use crate::render::ui::{PackageListing, PackagePreviewData};
use serde::Deserialize;
use std::sync::Arc;

#[derive(Deserialize, Default)]
#[serde(default)]
struct StatusMeta {
    category: String,
    icon_texture_path: Option<String>,
    name: String,
    long_name: String,
    description: String,
    flag_name: String,
    mutual_exclusions: Vec<String>,
    blocked_by: Vec<String>,
    blocks_flags: Vec<String>,
    blocks_actions: bool,
    blocks_mobility: bool,
    durations: Vec<FrameTime>,
    tags: Vec<String>,
}

#[derive(Default, Clone)]
pub struct StatusPackage {
    pub package_info: PackageInfo,
    pub icon_texture_path: Option<String>,
    pub name: Arc<str>,
    pub long_name: Arc<str>,
    pub description: Arc<str>,
    pub flag_name: Arc<str>,
    pub mutual_exclusions: Vec<String>,
    pub blocked_by: Vec<String>,
    pub blocks_flags: Vec<String>,
    pub blocks_actions: bool,
    pub blocks_mobility: bool,
    pub durations: Vec<FrameTime>,
}

impl Package for StatusPackage {
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
            preview_data: PackagePreviewData::Status,
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = Self {
            package_info,
            ..Default::default()
        };

        let meta: StatusMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        package.package_info.tags = meta.tags.into_iter().map(|s| s.into()).collect();

        if meta.category != "status" {
            log::error!(
                "Missing `category = \"status\"` in {:?}",
                package.package_info.toml_path
            );
        }

        let base_path = &package.package_info.base_path;

        package.icon_texture_path = meta.icon_texture_path.map(|p| base_path.clone() + &p);
        package.name = meta.name.into();
        package.long_name = if meta.long_name.is_empty() {
            package.name.clone()
        } else {
            meta.long_name.into()
        };
        package.description = meta.description.into();
        package.flag_name = meta.flag_name.into();
        package.mutual_exclusions = meta.mutual_exclusions;
        package.blocked_by = meta.blocked_by;
        package.blocks_flags = meta.blocks_flags;
        package.blocks_actions = meta.blocks_actions;
        package.blocks_mobility = meta.blocks_mobility;
        package.durations = meta.durations;

        if package.durations.is_empty() {
            package.durations.push(1);
        }

        package
    }
}
