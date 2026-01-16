use super::*;
use crate::resources::{AssetManager, LocalAssetManager, ResourcePaths};
use packets::structures::FileHash;
use std::sync::Arc;

pub struct ChildPackageInfo {
    pub parent_type: PackageCategory,
    pub parent_id: PackageId,
    pub id: PackageId,
    pub path: String,
}

#[derive(Clone)]
pub struct PackageInfo {
    pub id: PackageId,
    pub hash: FileHash,
    pub tags: Vec<Arc<str>>,
    pub shareable: bool,
    pub category: PackageCategory,
    pub namespace: PackageNamespace,
    pub base_path: String,
    pub script_path: String,
    pub toml_path: String,
    pub parent_package: Option<(PackageCategory, PackageId)>, // stores id, namespace should be the same
    pub child_id_path_pairs: Vec<(PackageId, String)>, // stores id and entry path, namespace should be the same
    pub requirements: Vec<(PackageCategory, PackageId)>, // stores id, namespace should be the same or fallback
}

impl Default for PackageInfo {
    fn default() -> Self {
        Self {
            id: Default::default(),
            hash: Default::default(),
            tags: Default::default(),
            shareable: true,
            category: Default::default(),
            namespace: Default::default(),
            base_path: Default::default(),
            script_path: Default::default(),
            toml_path: Default::default(),
            parent_package: Default::default(),
            child_id_path_pairs: Default::default(),
            requirements: Default::default(),
        }
    }
}

impl PackageInfo {
    pub fn has_tag(&self, tag: &str) -> bool {
        self.tags.iter().any(|t| &**t == tag)
    }

    pub fn local_only(&self) -> bool {
        matches!(
            self.category,
            PackageCategory::Resource | PackageCategory::Encounter
        ) && self.child_id_path_pairs.is_empty()
    }

    pub fn triplet(&self) -> (PackageCategory, PackageNamespace, PackageId) {
        (self.category, self.namespace, self.id.clone())
    }

    pub fn is_virtual(&self) -> bool {
        self.base_path.starts_with(ResourcePaths::VIRTUAL_PREFIX)
    }

    pub fn child_packages(&self) -> impl Iterator<Item = ChildPackageInfo> + '_ {
        self.child_id_path_pairs
            .iter()
            .cloned()
            .map(|(id, path)| ChildPackageInfo {
                parent_id: self.id.clone(),
                parent_type: self.category,
                id,
                path,
            })
    }

    // returns the [package] table and processes properties shared among every package type
    pub(crate) fn parse_toml(&mut self, assets: &LocalAssetManager) -> Option<toml::Table> {
        let toml_text = assets.text(&self.toml_path);

        if toml_text.is_empty() {
            // assume no file / not a mod
            // attempting to access the file will already provide a warning
            return None;
        }

        let meta_table: toml::Table = match toml_text.parse() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!(
                    "Failed to parse {:?}:\n{e}",
                    ResourcePaths::shorten(&self.toml_path)
                );
                return None;
            }
        };

        if let Some(dep_table) = meta_table.get("dependencies") {
            self.process_dependencies(dep_table, "augments", PackageCategory::Augment);
            self.process_dependencies(dep_table, "cards", PackageCategory::Card);
            self.process_dependencies(dep_table, "characters", PackageCategory::Character);
            self.process_dependencies(dep_table, "libraries", PackageCategory::Library);
            self.process_dependencies(dep_table, "statuses", PackageCategory::Status);
            self.process_dependencies(dep_table, "tile_states", PackageCategory::TileState);
        }

        if let Some(defines_table) = meta_table.get("defines") {
            self.process_defines(defines_table, "characters", PackageCategory::Character);
        }

        let package_table = meta_table
            .get("package")
            .and_then(|table| table.as_table().cloned());

        let Some(package_table) = package_table else {
            log::error!(
                "Missing [package] section in {:?}",
                ResourcePaths::shorten(&self.toml_path)
            );
            return None;
        };

        self.id = package_table.get("id")?.as_str()?.to_string().into();

        Some(package_table)
    }

    pub(crate) fn process_dependencies(
        &mut self,
        dependency_table: &toml::Value,
        key: &str,
        category: PackageCategory,
    ) {
        use toml::Value;

        if let Some(ids) = dependency_table.get(key).and_then(Value::as_array) {
            self.requirements.extend(
                ids.iter()
                    .flat_map(|id| id.as_str())
                    .map(|id| (category, id.into())),
            )
        }
    }

    pub(crate) fn process_defines(
        &mut self,
        defines_table: &toml::Value,
        key: &str,
        category: PackageCategory,
    ) {
        use toml::Value;

        if let Some(list) = defines_table.get(key).and_then(Value::as_array) {
            self.child_id_path_pairs.extend(
                list.iter()
                    .flat_map(|table| {
                        Some((table.get("id")?.as_str()?, table.get("path")?.as_str()?))
                    })
                    .map(|(id, path)| (id.into(), self.base_path.clone() + path)),
            );

            self.requirements.extend(
                list.iter()
                    .flat_map(|table| table.get("id")?.as_str())
                    .map(|id| (category, id.into())),
            );
        }
    }
}
