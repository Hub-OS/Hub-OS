use super::{ChildPackageInfo, Package, PackageId, PackageInfo, PackageNamespace};
use crate::resources::{LocalAssetManager, ResourcePaths};
use packets::structures::FileHash;
use std::collections::HashMap;

pub use packets::structures::PackageCategory;

pub struct PackageManager<T> {
    package_category: PackageCategory,
    package_maps: HashMap<PackageNamespace, HashMap<PackageId, T>>,
    package_ids: Vec<PackageId>,
}

impl<T: Package> PackageManager<T> {
    pub fn new(package_category: PackageCategory) -> Self {
        Self {
            package_category,
            package_maps: HashMap::new(),
            package_ids: Vec::new(),
        }
    }

    pub fn category(&self) -> PackageCategory {
        self.package_category
    }

    pub fn namespaces(&self) -> impl Iterator<Item = PackageNamespace> + '_ {
        self.package_maps.keys().cloned()
    }

    pub fn package_ids(&self, ns: PackageNamespace) -> impl Iterator<Item = &PackageId> + '_ {
        self.package_maps
            .get(&ns)
            .into_iter()
            .flat_map(|packages| packages.keys())
    }

    pub fn packages(&self, ns: PackageNamespace) -> impl Iterator<Item = &T> {
        self.package_maps
            .get(&ns)
            .into_iter()
            .flat_map(|packages| packages.values())
    }

    pub fn package(&self, ns: PackageNamespace, id: &PackageId) -> Option<&T> {
        self.package_maps.get(&ns)?.get(id)
    }

    pub fn package_or_fallback(&self, ns: PackageNamespace, id: &PackageId) -> Option<&T> {
        ns.find_with_fallback(|ns| self.package(ns, id))
    }

    pub fn child_packages(&self, ns: PackageNamespace) -> Vec<ChildPackageInfo> {
        self.package_ids
            .iter()
            .flat_map(|id| self.package(ns, id))
            .flat_map(|package| {
                let package_info = package.package_info();

                package_info
                    .child_id_path_pairs
                    .iter()
                    .cloned()
                    .map(|(id, path)| ChildPackageInfo {
                        parent_id: package_info.id.clone(),
                        parent_type: self.package_category,
                        id,
                        path,
                    })
            })
            .collect()
    }

    pub fn load_packages_in_folder<F>(
        &mut self,
        assets: &LocalAssetManager,
        namespace: PackageNamespace,
        path: &str,
        mut callback: F,
    ) where
        F: FnMut(usize, usize),
    {
        use std::fs;

        let _ = fs::create_dir_all(path);

        let paths: Vec<_> = fs::read_dir(path)
            .unwrap()
            .flatten()
            .filter_map(|entry| entry.path().to_str().map(|path| path.to_string()))
            .collect();

        let total = paths.len();

        for (i, base_path) in paths.into_iter().enumerate() {
            self.load_package(assets, namespace, &base_path);
            callback(i, total);
        }
    }

    pub fn load_child_packages<I>(&mut self, namespace: PackageNamespace, child_packages: I)
    where
        I: std::iter::IntoIterator<Item = ChildPackageInfo>,
    {
        for child_package_info in child_packages.into_iter() {
            self.load_child_package(namespace, &child_package_info);
        }
    }

    pub fn load_child_package(
        &mut self,
        namespace: PackageNamespace,
        child_package_info: &ChildPackageInfo,
    ) -> bool {
        let Some(mut package_info) =
            self.generate_package_info(namespace, &child_package_info.path)
        else {
            return false;
        };

        let parent_tuple = (
            child_package_info.parent_type,
            child_package_info.parent_id.clone(),
        );

        package_info.id = child_package_info.id.clone();
        package_info.parent_package = Some(parent_tuple.clone());
        package_info.requirements.push(parent_tuple);

        self.internal_load_package(package_info, toml::Table::new())
            .is_some()
    }

    pub fn load_package(
        &mut self,
        assets: &LocalAssetManager,
        namespace: PackageNamespace,
        path_str: &str,
    ) -> Option<&PackageInfo> {
        let path_string = ResourcePaths::clean(path_str);

        let mut package_info = self.generate_package_info(namespace, &path_string)?;

        // parse toml before zipping to resolve child packages
        // and provide enough details for correct output from `package_info.local_only()`
        let package_table = package_info.parse_toml(assets)?;

        package_info.hash = Self::zip_and_hash(&package_info)?;

        self.internal_load_package(package_info, package_table)
    }

    pub fn load_virtual_package(
        &mut self,
        assets: &LocalAssetManager,
        namespace: PackageNamespace,
        hash: FileHash,
    ) -> Option<&PackageInfo> {
        let Some(zip_meta) = assets.virtual_zip_meta(&hash) else {
            log::error!("Package {hash} has not been loaded as a virtual zip");
            return None;
        };

        let mut package_info = self.generate_package_info(namespace, &zip_meta.virtual_prefix)?;
        package_info.hash = hash;

        let package_table = package_info.parse_toml(assets)?;
        let package_info = self.internal_load_package(package_info, package_table)?;

        // increment usage after we're certain it's in use
        assets.add_virtual_zip_use(&hash);

        Some(package_info)
    }

    fn generate_package_info(
        &self,
        namespace: PackageNamespace,
        path: &str,
    ) -> Option<PackageInfo> {
        let Some((base_path, script_path)) = Self::resolve_package_paths(path) else {
            log::error!(
                "Failed to resolve parent folder for {:?}?",
                ResourcePaths::shorten(path)
            );
            return None;
        };

        Some(PackageInfo {
            id: PackageId::new_blank(),
            hash: FileHash::ZERO,
            shareable: true,
            category: self.package_category,
            namespace,
            base_path: base_path.clone(),
            script_path,
            toml_path: base_path + "package.toml",
            parent_package: None,
            child_id_path_pairs: Vec::new(),
            requirements: Vec::new(),
        })
    }

    fn resolve_package_paths(path: &str) -> Option<(String, String)> {
        let base_path;
        let script_path;

        let path = if ResourcePaths::is_absolute(path) {
            path.to_string()
        } else {
            ResourcePaths::data_folder_absolute(path)
        };

        if path.to_lowercase().ends_with(".lua") {
            // assume file
            // not actually checking since we deal with virtual files
            let path = ResourcePaths::clean(&path);
            let file_parent_path = ResourcePaths::parent(&path)?.to_string();

            base_path = file_parent_path;
            script_path = path;
        } else {
            base_path = ResourcePaths::clean_folder(&path);
            script_path = base_path.clone() + "entry.lua";
        }

        Some((base_path, script_path))
    }

    fn zip_and_hash(package_info: &PackageInfo) -> Option<FileHash> {
        if package_info.parent_package.is_some() {
            log::error!(
                "{:?}: Child packages should not be calling PackageManager::zip_and_hash",
                package_info.base_path
            );
            return None;
        }

        if !package_info.shareable {
            // file can't be downloaded or shared by client, no need to resolve hash
            return Some(FileHash::ZERO);
        }

        let data = match packets::zip::compress(&package_info.base_path) {
            Ok(data) => data,
            Err(e) => {
                log::error!("Failed to zip package {:?}: {e}", package_info.base_path);
                return None;
            }
        };

        let hash = FileHash::hash(&data);

        if package_info.local_only() {
            // file won't be sent, no need to save a zipped copy
            return Some(hash);
        }

        let mod_cache_folder = ResourcePaths::mod_cache_folder();
        let path = format!("{mod_cache_folder}{hash}.zip");

        if std::path::Path::new(&path).exists() {
            // file already cached
            return Some(hash);
        }

        if let Err(e) = std::fs::create_dir_all(&mod_cache_folder) {
            log::error!("Failed to create cache folder {mod_cache_folder:?}: {e}",);
        }

        if let Err(e) = std::fs::write(&path, data) {
            log::error!("Failed to cache package zip {path:?}: {e}");
            return None;
        }

        Some(hash)
    }

    fn internal_load_package(
        &mut self,
        package_info: PackageInfo,
        package_table: toml::Table,
    ) -> Option<&PackageInfo> {
        let package = T::load_new(package_info, package_table);

        let package_info = package.package_info();
        let package_id = package_info.id.clone();

        if package_id.is_blank() {
            log::error!(
                "Package is missing a package_id {:?}",
                ResourcePaths::shorten(&package_info.script_path)
            );
            return None;
        }

        let packages = self
            .package_maps
            .entry(package_info.namespace)
            .or_insert_with(|| HashMap::new());

        if packages.contains_key(&package_id) {
            log::error!("Duplicate package_id {package_id:?}");
            return None;
        }

        packages.insert(package_id.clone(), package);

        if !self.package_ids.contains(&package_id) {
            self.package_ids.push(package_id.clone());
        }

        packages
            .get(&package_id)
            .map(|package| package.package_info())
    }

    pub fn unload_package(
        &mut self,
        assets: &LocalAssetManager,
        namespace: PackageNamespace,
        id: &PackageId,
    ) {
        let Some(package_map) = self.package_maps.get_mut(&namespace) else {
            return;
        };

        let Some(package) = package_map.remove(id) else {
            return;
        };

        let package_info = package.package_info();

        if package_info.is_virtual() && package_info.hash != FileHash::ZERO {
            assets.remove_virtual_zip_use(&package_info.hash);
        }
    }

    pub fn remove_namespace(&mut self, assets: &LocalAssetManager, namespace: PackageNamespace) {
        let Some(packages) = self.package_maps.remove(&namespace) else {
            return;
        };

        for (_, package) in packages {
            let package_info = package.package_info();

            if package_info.is_virtual() && package_info.hash != FileHash::ZERO {
                assets.remove_virtual_zip_use(&package_info.hash);
            }
        }
    }
}
