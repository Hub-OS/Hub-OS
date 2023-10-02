use super::*;
use crate::render::ui::{PackageListing, PackagePreviewData};

#[derive(Default, Clone)]
pub struct CharacterPackage {
    pub package_info: PackageInfo,
}

impl Package for CharacterPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            id: self.package_info.id.clone(),
            name: String::new(),
            description: String::new(),
            creator: String::new(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::Unknown,
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, _package_table: toml::Table) -> Self {
        Self { package_info }
    }
}
