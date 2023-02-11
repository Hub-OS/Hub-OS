use super::*;
use crate::render::ui::{PackageListing, PackagePreviewData};
use serde::Deserialize;

#[derive(Deserialize, Default)]
#[serde(default)]
struct BattleMeta {
    category: String,
    name: String,
    description: String,
    preview_texture_path: String,
}

#[derive(Default, Clone)]
pub struct BattlePackage {
    pub package_info: PackageInfo,
    pub name: String,
    pub description: String,
    pub preview_texture_path: String,
}

impl Package for BattlePackage {
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
            preview_data: PackagePreviewData::Battle,
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = BattlePackage {
            package_info,
            ..Default::default()
        };

        let meta: BattleMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "battle" {
            log::error!(
                "missing `category = \"battle\"` in {:?}",
                package.package_info.toml_path
            );
        }

        let base_path = &package.package_info.base_path;

        package.name = meta.name;
        package.description = meta.description;
        package.preview_texture_path = base_path.clone() + &meta.preview_texture_path;

        package
    }
}
