use crate::render::ui::{PackageListing, PackagePreviewData};

use super::*;
use serde::Deserialize;

#[derive(Deserialize, Default)]
#[serde(default)]
struct LibraryMeta {
    category: String,
    name: String,
    description: String,
}

#[derive(Default, Clone)]
pub struct LibraryPackage {
    pub package_info: PackageInfo,
    name: String,
    description: String,
}

impl Package for LibraryPackage {
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
            preview_data: PackagePreviewData::Library,
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = Self {
            package_info,
            name: String::new(),
            description: String::new(),
        };

        let meta: LibraryMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "library" && meta.category != "pack" {
            log::error!(
                "Missing `category = \"library\"` in {:?}",
                package.package_info.toml_path
            );
        }

        package.name = meta.name;
        package.description = meta.description;

        package
    }
}
