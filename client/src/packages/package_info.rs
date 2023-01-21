use super::*;
use packets::structures::FileHash;

pub struct ChildPackageInfo {
    pub parent_type: PackageCategory,
    pub parent_id: PackageId,
    pub id: PackageId,
    pub path: String,
}

#[derive(Default, Clone)]
pub struct PackageInfo {
    pub id: PackageId,
    pub hash: FileHash,
    pub package_category: PackageCategory,
    pub namespace: PackageNamespace,
    pub base_path: String,
    pub script_path: String,
    pub parent_package: Option<(PackageCategory, PackageId)>, // stores id, namespace should be the same
    pub child_id_path_pairs: Vec<(PackageId, String)>, // stores id and entry path, namespace should be the same
    pub requirements: Vec<(PackageCategory, PackageId)>, // stores id, namespace should be the same or fallback
}

impl PackageInfo {
    pub fn triplet(&self) -> (PackageCategory, PackageNamespace, PackageId) {
        (self.package_category, self.namespace, self.id.clone())
    }

    pub fn child_packages(&self) -> impl Iterator<Item = ChildPackageInfo> + '_ {
        self.child_id_path_pairs
            .iter()
            .cloned()
            .map(|(id, path)| ChildPackageInfo {
                parent_id: self.id.clone(),
                parent_type: self.package_category,
                id,
                path,
            })
    }
}
