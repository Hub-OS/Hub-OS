use super::PackageInfo;
use crate::render::ui::PackageListing;
use packets::structures::FileHash;

pub trait Package {
    fn package_info(&self) -> &PackageInfo;

    fn update_hash(&mut self, hash: FileHash);

    fn create_package_listing(&self) -> PackageListing;

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self;
}
