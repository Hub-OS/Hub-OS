use super::*;
use crate::bindable::BlockColor;
use crate::bindable::SwitchDriveSlot;
use crate::render::ui::{PackageListing, PackagePreviewData};
use serde::Deserialize;
use std::borrow::Cow;
use std::sync::Arc;

#[derive(Deserialize, Default)]
#[serde(default)]
struct AugmentMeta {
    category: String,
    name: String,
    long_name: String,
    description: String,
    long_description: String,
    priority: Option<bool>,
    health_boost: i32,
    attack_boost: i8,
    rapid_boost: i8,
    charge_boost: i8,
    mega_boost: i8,
    giga_boost: i8,
    dark_boost: i8,
    hand_size_boost: i8,
    tags: Vec<Cow<'static, str>>,

    // switch drive specific
    slot: Option<SwitchDriveSlot>,

    // block specific
    colors: Vec<String>,
    flat: bool,
    shape: Option<Vec<Vec<u8>>>,
    byproducts: Vec<PackageId>,
    prevent_byproducts: bool,
    limit: Option<usize>,
}

#[derive(Default, Clone)]
pub struct AugmentPackage {
    pub package_info: PackageInfo,
    pub name: Arc<str>,
    pub long_name: Arc<str>,
    pub description: Arc<str>,
    pub long_description: Arc<str>,
    pub health_boost: i32,
    pub attack_boost: i8,
    pub rapid_boost: i8,
    pub charge_boost: i8,
    pub mega_boost: i8,
    pub giga_boost: i8,
    pub dark_boost: i8,
    pub hand_size_boost: i8,
    pub priority: bool,
    pub tags: Vec<Cow<'static, str>>,

    // switch drive specific
    pub slot: Option<SwitchDriveSlot>,

    // block specific
    pub has_shape: bool,
    pub is_flat: bool,
    pub block_colors: Vec<BlockColor>,
    pub shape: [bool; 5 * 5],
    pub byproducts: Vec<PackageId>,
    pub prevent_byproducts: bool,
    pub limit: usize,
}

impl AugmentPackage {
    pub fn exists_at(&self, rotation: u8, position: (usize, usize)) -> bool {
        if position.0 >= 5 || position.1 >= 5 {
            return false;
        }

        let (x, y) = match rotation {
            1 => (position.1, 4 - position.0),
            2 => (4 - position.0, 4 - position.1),
            3 => (4 - position.1, position.0),
            _ => (position.0, position.1),
        };

        self.shape.get(y * 5 + x) == Some(&true)
    }
}

impl Package for AugmentPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            local: true,
            id: self.package_info.id.clone(),
            name: self.name.clone(),
            long_name: self.long_name.clone(),
            description: if !self.long_description.is_empty() {
                self.long_description.clone()
            } else {
                self.description.clone()
            },
            creator: Default::default(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::Augment {
                slot: self.slot,
                flat: self.is_flat,
                colors: self.block_colors.clone(),
                shape: self.has_shape.then_some(self.shape),
            },
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = AugmentPackage {
            package_info,
            ..Default::default()
        };

        let meta: AugmentMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "augment" {
            log::error!(
                "Missing `category = \"augment\"` in {:?}",
                package.package_info.toml_path
            );
        }

        package.name = meta.name.into();
        package.long_name = if meta.long_name.is_empty() {
            package.name.clone()
        } else {
            meta.long_name.into()
        };
        package.description = meta.description.into();
        package.long_description = meta.long_description.into();
        package.health_boost = meta.health_boost;
        package.attack_boost = meta.attack_boost;
        package.rapid_boost = meta.rapid_boost;
        package.charge_boost = meta.charge_boost;
        package.mega_boost = meta.mega_boost;
        package.giga_boost = meta.giga_boost;
        package.dark_boost = meta.dark_boost;
        package.hand_size_boost = meta.hand_size_boost;
        package.priority = meta.priority.unwrap_or_default();
        package.tags = meta.tags;

        // switch drive specific
        package.slot = meta.slot;

        // block specific
        package.has_shape = meta.shape.is_some();
        package.is_flat = meta.flat;
        package.block_colors = meta.colors.into_iter().map(BlockColor::from).collect();
        package.byproducts = meta.byproducts;
        package.prevent_byproducts = meta.prevent_byproducts;
        package.limit = meta.limit.unwrap_or(9);

        // block tags
        if package.has_shape {
            package.tags.push(Cow::Borrowed("BLOCK"));

            if package.is_flat {
                package.tags.push(Cow::Borrowed("FLAT_BLOCK"));
            }
        }

        if let Some(shape) = meta.shape {
            let flattened_shape: Vec<_> = shape.into_iter().flatten().collect();

            if flattened_shape.len() != package.shape.len() {
                log::error!(
                    "Expected a 5x5 shape (5 lists of 5 numbers) in {:?}",
                    package.package_info.toml_path
                );
            }

            for (i, n) in flattened_shape.into_iter().enumerate() {
                package.shape[i] = n != 0;
            }
        }

        package
    }
}
