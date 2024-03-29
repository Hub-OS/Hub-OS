use super::*;
use crate::bindable::{CardClass, CardProperties};
use crate::render::ui::{PackageListing, PackagePreviewData};
use crate::render::SpriteColorQueue;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::{GameIO, Sprite, Texture, UVec2, Vec2};
use serde::Deserialize;
use std::sync::Arc;

#[derive(Deserialize, Default)]
#[serde(default)]
struct CardMeta {
    category: String,
    icon_texture_path: String,
    preview_texture_path: String,
    codes: Vec<String>,
    regular_allowed: Option<bool>,
    hidden: bool,

    // card properties
    name: String,
    description: String,
    long_description: String,
    damage: i32,
    recover: i32,
    element: String,
    secondary_element: String,
    card_class: String,
    limit: Option<usize>,
    hit_flags: Option<Vec<String>>,
    can_boost: Option<bool>,
    time_freeze: bool,
    skip_time_freeze_intro: bool,
    prevent_time_freeze_counter: bool,
    conceal: bool,
    tags: Vec<String>,
}

#[derive(Default, Clone)]
pub struct CardPackage {
    pub package_info: PackageInfo,
    pub icon_texture_path: String,
    pub preview_texture_path: String,
    pub card_properties: CardProperties<Vec<String>>,
    pub description: String,
    pub long_description: String,
    pub default_codes: Vec<String>,
    pub regular_allowed: bool,
    pub hidden: bool,
    pub limit: usize,
}

impl Package for CardPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            id: self.package_info.id.clone(),
            name: self.card_properties.short_name.to_string(),
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
        package.hidden = meta.hidden;
        package.limit = meta.limit.unwrap_or(5);

        // card properties
        package.card_properties.package_id = package.package_info.id.clone();
        package.card_properties.short_name = meta.name.into();
        package.card_properties.damage = meta.damage;
        package.card_properties.recover = meta.recover;
        package.card_properties.element = meta.element.into();
        package.card_properties.secondary_element = meta.secondary_element.into();
        package.card_properties.card_class = meta.card_class.into();

        if let Some(hit_flags) = meta.hit_flags {
            package.card_properties.hit_flags = hit_flags;
        }

        if let Some(can_boost) = meta.can_boost {
            package.card_properties.can_boost = can_boost;
        }

        package.card_properties.time_freeze = meta.time_freeze;
        package.card_properties.skip_time_freeze_intro = meta.skip_time_freeze_intro;
        package.card_properties.prevent_time_freeze_counter = meta.prevent_time_freeze_counter;
        package.card_properties.conceal = meta.conceal;
        package.card_properties.tags = meta.tags;

        // default for regular_allowed is based on card class
        let card_class = package.card_properties.card_class;
        package.regular_allowed = meta
            .regular_allowed
            .unwrap_or(card_class == CardClass::Standard);

        package
    }
}

impl CardPackage {
    // used in netplay, luckily we shouldnt see what remotes have, so using local namespace is fine
    pub fn draw_icon(
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        package_id: &PackageId,
        position: Vec2,
    ) {
        let (icon_texture, _) = Self::icon_texture(game_io, package_id);

        let mut sprite = Sprite::new(game_io, icon_texture);
        sprite.set_position(position);
        sprite_queue.draw_sprite(&sprite);
    }

    pub fn icon_texture<'a>(
        game_io: &'a GameIO,
        package_id: &PackageId,
    ) -> (Arc<Texture>, &'a str) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;
        let ns = PackageNamespace::Local;

        if let Some(package) = package_manager.package_or_override(ns, package_id) {
            let icon_texture = assets.texture(game_io, &package.icon_texture_path);

            if icon_texture.size() == UVec2::new(14, 14) {
                return (icon_texture, &package.icon_texture_path);
            }
        };

        let path = ResourcePaths::CARD_ICON_MISSING;
        (assets.texture(game_io, path), path)
    }

    pub fn preview_texture<'a>(
        game_io: &'a GameIO,
        package_id: &PackageId,
    ) -> (Arc<Texture>, &'a str) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;
        let ns = PackageNamespace::Local;

        if let Some(package) = package_manager.package_or_override(ns, package_id) {
            let path = package.preview_texture_path.as_str();
            let texture = assets.texture(game_io, path);

            if texture.size() != UVec2::ONE {
                return (texture, path);
            }
        }

        let path = ResourcePaths::CARD_ICON_MISSING;
        (assets.texture(game_io, path), path)
    }
}
