use super::*;
use serde::Deserialize;

#[derive(Deserialize, Default)]
#[serde(default)]
struct LibraryMeta {
    category: String,
}

#[derive(Default, Clone)]
pub struct LibraryPackage {
    pub package_info: PackageInfo,
}

impl Package for LibraryPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let package = Self { package_info };

        let meta: LibraryMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "library" {
            log::error!(
                "missing `category = \"library\"` in {:?}",
                package.package_info.toml_path
            );
        }

        package
    }
}
