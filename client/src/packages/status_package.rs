use super::*;
use crate::render::ui::{PackageListing, PackagePreviewData};
use crate::render::FrameTime;
use serde::Deserialize;

#[derive(Deserialize, Default)]
#[serde(default)]
struct StatusMeta {
    category: String,
    name: String,
    description: String,
    flag_name: String,
    blocks_flags: Vec<String>,
    blocked_by: Vec<String>,
    blocks_actions: bool,
    blocks_mobility: bool,
    durations: Vec<FrameTime>,
}

#[derive(Default, Clone)]
pub struct StatusPackage {
    pub package_info: PackageInfo,
    pub name: String,
    pub description: String,
    pub flag_name: String,
    pub blocks_flags: Vec<String>,
    pub blocked_by: Vec<String>,
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
            id: self.package_info.id.clone(),
            name: self.name.clone(),
            description: self.description.clone(),
            creator: String::new(),
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

        if meta.category != "status" {
            log::error!(
                "Missing `category = \"status\"` in {:?}",
                package.package_info.toml_path
            );
        }

        package.name = meta.name;
        package.description = meta.description;
        package.flag_name = meta.flag_name;
        package.blocks_flags = meta.blocks_flags;
        package.blocked_by = meta.blocked_by;
        package.blocks_actions = meta.blocks_actions;
        package.blocks_mobility = meta.blocks_mobility;
        package.durations = meta.durations;

        if package.durations.is_empty() {
            package.durations.push(1);
        }

        package
    }
}
