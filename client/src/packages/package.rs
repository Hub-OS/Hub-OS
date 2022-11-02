use super::PackageInfo;
use crate::resources::AssetManager;

pub trait Package {
    fn load_new(assets: &impl AssetManager, package_info: PackageInfo) -> Self;

    fn package_info(&self) -> &PackageInfo;

    fn package_info_mut(&mut self) -> &mut PackageInfo;
}
