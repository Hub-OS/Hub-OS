use super::PackageInfo;
use crate::resources::LocalAssetManager;

pub trait Package {
    fn load_new(assets: &LocalAssetManager, package_info: PackageInfo) -> Self;

    fn package_info(&self) -> &PackageInfo;

    fn package_info_mut(&mut self) -> &mut PackageInfo;
}
