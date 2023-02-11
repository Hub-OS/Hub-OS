use super::PackageInfo;
use crate::render::ui::PackageListing;

pub trait Package {
    fn package_info(&self) -> &PackageInfo;

    fn package_info_mut(&mut self) -> &mut PackageInfo;

    fn create_package_listing(&self) -> PackageListing;

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self;
}
