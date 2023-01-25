use super::PackageInfo;

pub trait Package {
    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self;

    fn package_info(&self) -> &PackageInfo;

    fn package_info_mut(&mut self) -> &mut PackageInfo;
}
