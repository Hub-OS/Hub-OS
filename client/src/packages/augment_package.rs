use super::*;
use crate::bindable::BlockColor;
use crate::bindable::SwitchDriveSlot;
use crate::render::ui::{PackageListing, PackagePreviewData};
use crate::saves::BlockShape;
use packets::structures::FileHash;
use serde::Deserialize;
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
    deck_boost: i8,
    hand_size_boost: i8,
    visible_to_tagged: Vec<String>,
    tags: Vec<String>,

    // switch drive specific
    slot: Option<SwitchDriveSlot>,

    // block specific
    colors: Vec<String>,
    flat: bool,
    shape: Option<Vec<Vec<u8>>>,
    shapes: Vec<Vec<Vec<u8>>>,
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
    pub visible_to_tagged: Vec<String>,
    pub health_boost: i32,
    pub attack_boost: i8,
    pub rapid_boost: i8,
    pub charge_boost: i8,
    pub mega_boost: i8,
    pub giga_boost: i8,
    pub dark_boost: i8,
    pub deck_boost: i8,
    pub hand_size_boost: i8,
    pub priority: bool,

    // switch drive specific
    pub slot: Option<SwitchDriveSlot>,

    // block specific
    pub is_flat: bool,
    pub block_colors: Vec<BlockColor>,
    pub shapes: Vec<BlockShape>,
    pub byproducts: Vec<PackageId>,
    pub prevent_byproducts: bool,
    pub limit: usize,
}

impl AugmentPackage {
    pub fn exists_at(&self, variant: usize, rotation: u8, position: (i8, i8)) -> bool {
        let Some(shape) = self.shapes.get(variant) else {
            return false;
        };

        shape.exists_at(rotation, position)
    }
}

impl Package for AugmentPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn update_hash(&mut self, hash: FileHash) {
        self.package_info.hash = hash;
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
                shape: self.shapes.first().copied(),
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

        package.package_info.tags = meta.tags.into_iter().map(|s| s.into()).collect();

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
        package.visible_to_tagged = meta.visible_to_tagged;
        package.health_boost = meta.health_boost;
        package.attack_boost = meta.attack_boost;
        package.rapid_boost = meta.rapid_boost;
        package.charge_boost = meta.charge_boost;
        package.mega_boost = meta.mega_boost;
        package.giga_boost = meta.giga_boost;
        package.dark_boost = meta.dark_boost;
        package.deck_boost = meta.deck_boost;
        package.hand_size_boost = meta.hand_size_boost;
        package.priority = meta.priority.unwrap_or_default();

        // switch drive specific
        package.slot = meta.slot;

        // block specific
        package.is_flat = meta.flat;
        package.block_colors = meta.colors.into_iter().map(BlockColor::from).collect();
        package.byproducts = meta.byproducts;
        package.prevent_byproducts = meta.prevent_byproducts;
        package.limit = meta.limit.unwrap_or(9);

        // resolve block shapes
        let flatten_shape = |shape: Vec<Vec<u8>>| {
            BlockShape::try_from(shape)
                .inspect_err(|err| log::error!("In {:?}:\n{err}", package.package_info.toml_path))
                .ok()
        };

        if let Some(shape) = meta.shape {
            package.shapes.extend(flatten_shape(shape));
        }

        package
            .shapes
            .extend(meta.shapes.into_iter().flat_map(flatten_shape));

        // block tags
        if !package.shapes.is_empty() {
            static BLOCK_TAG: std::sync::LazyLock<Arc<str>> =
                std::sync::LazyLock::new(|| Arc::from("BLOCK"));
            static FLAT_BLOCK_TAG: std::sync::LazyLock<Arc<str>> =
                std::sync::LazyLock::new(|| Arc::from("BLOCK"));

            package.package_info.tags.push(BLOCK_TAG.clone());

            if package.is_flat {
                package.package_info.tags.push(FLAT_BLOCK_TAG.clone());
            }
        }

        package
    }
}
