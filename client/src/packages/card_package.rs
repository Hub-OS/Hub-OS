use super::*;
use crate::bindable::{CardProperties, HitFlag};
use crate::render::ui::{PackageListing, PackagePreviewData};
use serde::Deserialize;

#[derive(Deserialize, Default)]
#[serde(default)]
struct CardMeta {
    category: String,
    icon_texture_path: String,
    preview_texture_path: String,
    codes: Vec<String>,

    // card properties
    name: String,
    description: String,
    long_description: String,
    damage: i32,
    element: String,
    secondary_element: String,
    card_class: String,
    limit: Option<usize>,
    hit_flags: Option<Vec<String>>,
    can_boost: Option<bool>,
    time_freeze: bool,
    skip_time_freeze_intro: bool,
    meta_classes: Vec<String>,
}

#[derive(Default, Clone)]
pub struct CardPackage {
    pub package_info: PackageInfo,
    pub icon_texture_path: String,
    pub preview_texture_path: String,
    pub card_properties: CardProperties,
    pub description: String,
    pub long_description: String,
    pub limit: usize,
    pub default_codes: Vec<String>,
}

impl Package for CardPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn package_info_mut(&mut self) -> &mut PackageInfo {
        &mut self.package_info
    }

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            id: self.package_info.id.clone(),
            name: self.card_properties.short_name.clone(),
            description: if !self.long_description.is_empty() {
                self.long_description.clone()
            } else {
                self.description.clone()
            },
            creator: String::new(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::Card {
                codes: self.default_codes.clone(),
                element: self.card_properties.element,
                secondary_element: self.card_properties.secondary_element,
                damage: self.card_properties.damage,
            },
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = CardPackage {
            package_info,
            ..Default::default()
        };

        let meta: CardMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        if meta.category != "card" {
            log::error!(
                "Missing `category = \"card\"` in {:?}",
                package.package_info.toml_path
            );
        }

        let base_path = &package.package_info.base_path;

        package.icon_texture_path = base_path.clone() + &meta.icon_texture_path;
        package.preview_texture_path = base_path.clone() + &meta.preview_texture_path;
        package.description = meta.description;
        package.long_description = meta.long_description;
        package.default_codes = meta.codes;
        package.limit = meta.limit.unwrap_or(5);

        // card properties
        package.card_properties.package_id = package.package_info.id.clone();
        package.card_properties.short_name = meta.name;
        package.card_properties.damage = meta.damage;
        package.card_properties.element = meta.element.into();
        package.card_properties.secondary_element = meta.secondary_element.into();
        package.card_properties.card_class = meta.card_class.into();

        if let Some(hit_flags) = meta.hit_flags {
            package.card_properties.hit_flags = hit_flags
                .into_iter()
                .map(|flag| HitFlag::from_str(&flag))
                .reduce(|acc, item| acc | item)
                .unwrap_or_default();
        }

        if let Some(can_boost) = meta.can_boost {
            package.card_properties.can_boost = can_boost;
        }

        package.card_properties.time_freeze = meta.time_freeze;
        package.card_properties.skip_time_freeze_intro = meta.skip_time_freeze_intro;
        package.card_properties.meta_classes = meta.meta_classes;

        package
    }
}
