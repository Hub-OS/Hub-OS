use super::*;

#[derive(Default, Clone)]
pub struct CharacterPackage {
    pub package_info: PackageInfo,
}

impl Package for CharacterPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
    }

    fn load_new(package_info: PackageInfo, _package_table: toml::Table) -> Self {
        Self { package_info }
    }
}
