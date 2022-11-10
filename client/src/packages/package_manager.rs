use super::{ChildPackageInfo, Package, PackageInfo, PackageNamespace};
use crate::resources::{LocalAssetManager, ResourcePaths};
use packets::structures::FileHash;
use std::collections::HashMap;

pub use packets::structures::PackageCategory;

pub struct PackageManager<T> {
    package_category: PackageCategory,
    package_maps: HashMap<PackageNamespace, HashMap<String, T>>,
    package_ids: Vec<String>,
}

impl<T: Package> PackageManager<T> {
    pub fn new(package_category: PackageCategory) -> Self {
        Self {
            package_category,
            package_maps: HashMap::new(),
            package_ids: Vec::new(),
        }
    }

    pub fn namespaces(&self) -> impl Iterator<Item = PackageNamespace> + '_ {
        self.package_maps.keys().cloned()
    }

    pub fn local_packages(&self) -> impl Iterator<Item = &String> + '_ {
        self.package_maps
            .get(&PackageNamespace::Local)
            .into_iter()
            .flat_map(|packages| packages.keys())
    }

    pub fn package(&self, ns: PackageNamespace, id: &str) -> Option<&T> {
        self.package_maps.get(&ns)?.get(id)
    }

    pub fn package_or_fallback(&self, ns: PackageNamespace, id: &str) -> Option<&T> {
        let package = self.package(ns, id);

        match ns {
            PackageNamespace::Remote(_) => {
                package.or_else(|| self.package_or_fallback(PackageNamespace::Server, id))
            }
            PackageNamespace::Server => {
                package.or_else(|| self.package_or_fallback(PackageNamespace::Local, id))
            }
            PackageNamespace::Local => package,
        }
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
        path: &str,
        mut callback: F,
    ) where
        F: FnMut(usize, usize),
    {
        use std::fs;

        fs::create_dir_all(path).unwrap();

        let paths: Vec<_> = fs::read_dir(path)
            .unwrap()
            .flatten()
            .filter_map(|entry| entry.path().to_str().map(|path| path.to_string()))
            .collect();

        let total = paths.len();

        for (i, base_path) in paths.into_iter().enumerate() {
            self.load_package(assets, PackageNamespace::Local, &base_path);
            callback(i, total);
        }
    }

    pub fn load_child_packages<I>(
        &mut self,
        assets: &LocalAssetManager,
        namespace: PackageNamespace,
        child_packages: I,
    ) where
        I: std::iter::IntoIterator<Item = ChildPackageInfo>,
    {
        for child_package_info in child_packages.into_iter() {
            self.load_child_package(assets, namespace, &child_package_info);
        }
    }

    pub fn load_child_package(
        &mut self,
        assets: &LocalAssetManager,
        namespace: PackageNamespace,
        child_package_info: &ChildPackageInfo,
    ) -> bool {
        let mut package_info = match self.generate_package_info(namespace, &child_package_info.path)
        {
            Some(package_info) => package_info,
            None => return false,
        };

        package_info.parent_package = Some((
            child_package_info.parent_type,
            child_package_info.parent_id.clone(),
        ));

        package_info.id = child_package_info.id.clone();

        self.internal_load_package(assets, package_info).is_some()
    }

    pub fn load_package(
        &mut self,
        assets: &LocalAssetManager,
        namespace: PackageNamespace,
        path_str: &str,
    ) -> Option<&PackageInfo> {
        let path_string = ResourcePaths::clean(path_str);

        let mut package_info = self.generate_package_info(namespace, &path_string)?;
        package_info.hash = Self::zip_and_hash(&package_info)?;

        self.internal_load_package(assets, package_info)
    }

    pub fn load_virtual_package(
        &mut self,
        assets: &LocalAssetManager,
        namespace: PackageNamespace,
        hash: FileHash,
    ) -> Option<&PackageInfo> {
        let zip_meta = match assets.virtual_zip_meta(&hash) {
            Some(zip_meta) => zip_meta,
            None => {
                log::error!("package {hash} has not been loaded as a virtual zip");
                return None;
            }
        };

        let mut package_info = self.generate_package_info(namespace, &zip_meta.virtual_prefix)?;
        package_info.hash = hash;

        let package_info = self.internal_load_package(assets, package_info)?;

        // increment usage after we're certain it's in use
        assets.add_virtual_zip_use(&hash);

        Some(package_info)
    }

    fn generate_package_info(
        &self,
        namespace: PackageNamespace,
        path: &str,
    ) -> Option<PackageInfo> {
        let (base_path, script_path) = match Self::resolve_package_paths(path) {
            Some(result) => result,
            None => {
                log::error!(
                    "failed to resolve parent folder for {:?}?",
                    ResourcePaths::shorten(path)
                );
                return None;
            }
        };

        Some(PackageInfo {
            id: String::new(),
            hash: FileHash::ZERO,
            package_category: self.package_category,
            namespace,
            base_path,
            script_path,
            parent_package: None,
            child_id_path_pairs: Vec::new(),
            requirements: Vec::new(),
        })
    }

    fn resolve_package_paths(path: &str) -> Option<(String, String)> {
        let base_path;
        let script_path;

        if path.to_lowercase().ends_with(".lua") {
            // assume file
            // not actually checking since we deal with virtual files
            let path = ResourcePaths::clean(path);
            let file_parent_path = ResourcePaths::parent(&path)?.to_string();

            base_path = file_parent_path;
            script_path = path;
        } else {
            base_path = ResourcePaths::clean_folder(path);
            script_path = base_path.clone() + "entry.lua";
        }

        Some((base_path, script_path))
    }

    fn zip_and_hash(package_info: &PackageInfo) -> Option<FileHash> {
        if package_info.parent_package.is_some() {
            log::error!(
                "{:?}: child packages should not be calling PackageManager::zip_and_hash",
                package_info.base_path
            );
            return None;
        }

        let data = match crate::zip::compress(&package_info.base_path) {
            Ok(data) => data,
            Err(e) => {
                log::error!("failed to zip package {:?}: {e}", package_info.base_path);
                return None;
            }
        };

        let hash = FileHash::hash(&data);
        let path = format!("{}{}.zip", ResourcePaths::MOD_CACHE_FOLDER, hash);

        if let Err(e) = std::fs::create_dir_all(ResourcePaths::MOD_CACHE_FOLDER) {
            log::error!(
                "failed to create cache folder {:?}: {e}",
                ResourcePaths::MOD_CACHE_FOLDER
            );
        }

        if let Err(e) = std::fs::write(&path, data) {
            log::error!("failed to cache package zip {:?}: {e}", path);
            return None;
        }

        Some(hash)
    }

    fn internal_load_package<'a>(
        &'a mut self,
        assets: &LocalAssetManager,
        package_info: PackageInfo,
    ) -> Option<&'a PackageInfo> {
        let package = T::load_new(assets, package_info);

        let package_info = package.package_info();
        let package_id = package_info.id.to_string();

        if package_id.is_empty() {
            log::error!(
                "package is missing a package_id {:?}",
                ResourcePaths::shorten(&package_info.script_path)
            );
            return None;
        }

        if !self.package_maps.contains_key(&package_info.namespace) {
            // no idea why i couldn't write this with a match expression
            self.package_maps
                .insert(package_info.namespace, HashMap::new());
        }

        let packages = self.package_maps.get_mut(&package_info.namespace).unwrap();

        if packages.contains_key(&package_id) {
            log::error!("duplicate package_id {:?}", package_id);
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

    pub fn remove_namespace(&mut self, assets: &LocalAssetManager, namespace: PackageNamespace) {
        let packages = match self.package_maps.remove(&namespace) {
            Some(packages) => packages,
            None => return,
        };

        for (_, package) in packages {
            let package_info = package.package_info();

            if package_info.hash != FileHash::ZERO {
                assets.remove_virtual_zip_use(&package_info.hash);
            }
        }
    }
}
