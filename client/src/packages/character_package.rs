use super::*;
use crate::render::ui::{PackageListing, PackagePreviewData};
use packets::structures::FileHash;

#[derive(Default, Clone)]
pub struct CharacterPackage {
    pub package_info: PackageInfo,
}

impl Package for CharacterPackage {
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
            name: Default::default(),
            long_name: Default::default(),
            description: Default::default(),
            creator: Default::default(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::Unknown,
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, _package_table: toml::Table) -> Self {
        Self { package_info }
    }
}
