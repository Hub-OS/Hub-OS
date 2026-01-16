use super::*;
use crate::CardRecipe;
use crate::bindable::{CardClass, CardProperties};
use crate::render::ui::{PackageListing, PackagePreviewData};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::structures::VecMap;
use framework::prelude::{GameIO, Sprite, Texture, UVec2, Vec2};
use serde::Deserialize;
use std::sync::Arc;

pub enum CardPackageStatusDuration {
    Level(usize),
    Duration(FrameTime),
}

#[derive(Deserialize, Default)]
#[serde(default)]
struct CardMeta {
    category: String,
    long_name: String,
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
    can_charge: Option<bool>,
    time_freeze: bool,
    skip_time_freeze_intro: bool,
    prevent_time_freeze_counter: bool,
    conceal: bool,
    dynamic_damage: bool,
    tags: Vec<String>,
}

#[derive(Default)]
pub struct CardPackage {
    pub package_info: PackageInfo,
    pub long_name: Arc<str>,
    pub icon_texture_path: String,
    pub preview_texture_path: String,
    pub description: Arc<str>,
    pub long_description: Arc<str>,
    pub card_properties: CardProperties<Vec<String>, VecMap<String, CardPackageStatusDuration>>,
    pub default_codes: Vec<String>,
    pub regular_allowed: bool,
    pub hidden: bool,
    pub limit: usize,
    pub recipes: Vec<CardRecipe>,
    pub search_name: String,
}

impl Package for CardPackage {
    fn package_info(&self) -> &PackageInfo {
        &self.package_info
    }

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            local: true,
            id: self.package_info.id.clone(),
            name: self.card_properties.short_name.clone().into(),
            long_name: self.long_name.clone(),
            description: if !self.long_description.is_empty() {
                self.long_description.clone()
            } else {
                self.description.clone()
            },
            creator: Default::default(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::Card {
                class: self.card_properties.card_class,
                codes: self.default_codes.clone(),
                element: self.card_properties.element,
                secondary_element: self.card_properties.secondary_element,
                damage: self.card_properties.damage,
                can_boost: self.card_properties.can_boost,
            },
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = CardPackage {
            package_info,
            ..Default::default()
        };

        // handle recipes
        if let Some(recipes) = package_table.get("recipes").and_then(|v| v.as_array()) {
            for recipe in recipes {
                if let Some(resolved) = CardRecipe::from_toml(recipe) {
                    package.recipes.push(resolved);
                }
            }
        }

        // handle status durations
        if let Some(durations_list) = package_table
            .get("status_durations")
            .and_then(|v| v.as_array())
        {
            for item in durations_list {
                let Some(key) = item.get("flag_name").and_then(|v| v.as_str()) else {
                    continue;
                };

                let duration = if let Some(level) = item.get("level").and_then(|v| v.as_integer()) {
                    if item.get("duration").is_some() {
                        log::warn!(
                            "Both level and duration specified for Hit.{key} in {:?}. Defaulting to level",
                            package.package_info.toml_path
                        )
                    }

                    CardPackageStatusDuration::Level(level.max(1) as usize)
                } else if let Some(frames) = item.get("duration").and_then(|v| v.as_integer()) {
                    CardPackageStatusDuration::Duration(frames)
                } else {
                    continue;
                };

                let durations = &mut package.card_properties.status_durations;
                durations.insert(key.to_string(), duration);
            }
        }

        // read as CardMeta
        let meta: CardMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        package.package_info.tags = meta.tags.into_iter().map(|s| s.into()).collect();

        if meta.category != "card" {
            log::error!(
                "Missing `category = \"card\"` in {:?}",
                package.package_info.toml_path
            );
        }

        let base_path = &package.package_info.base_path;

        package.long_name = if meta.long_name.is_empty() {
            (*meta.name.as_str()).into()
        } else {
            meta.long_name.into()
        };
        package.icon_texture_path = base_path.clone() + &meta.icon_texture_path;
        package.preview_texture_path = base_path.clone() + &meta.preview_texture_path;
        package.description = meta.description.into();
        package.long_description = meta.long_description.into();
        package.default_codes = meta.codes;
        package.hidden = meta.hidden;

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

        if let Some(can_charge) = meta.can_charge {
            package.card_properties.can_charge = can_charge;
        } else {
            package.card_properties.can_charge =
                package.card_properties.card_class != CardClass::Recipe;
        }

        package.card_properties.time_freeze = meta.time_freeze;
        package.card_properties.skip_time_freeze_intro = meta.skip_time_freeze_intro;
        package.card_properties.prevent_time_freeze_counter = meta.prevent_time_freeze_counter;
        package.card_properties.conceal = meta.conceal;
        package.card_properties.dynamic_damage = meta.dynamic_damage;
        package.card_properties.tags = package.package_info.tags.clone();

        // default for regular_allowed is based on card class
        let card_class = package.card_properties.card_class;
        package.regular_allowed = meta
            .regular_allowed
            .unwrap_or(card_class == CardClass::Standard);

        package.limit = meta.limit.unwrap_or_else(|| {
            if package.card_properties.card_class == CardClass::Recipe {
                1
            } else {
                5
            }
        });

        package.search_name = package.card_properties.short_name.to_lowercase()
            + "\0"
            + &package.long_name.to_lowercase();

        package
    }
}

impl CardPackage {
    // used in netplay, luckily we shouldnt see what remotes have, so using local namespace is fine
    pub fn draw_icon(
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        namespace: PackageNamespace,
        package_id: &PackageId,
        position: Vec2,
    ) {
        let (icon_texture, _) = Self::icon_texture(game_io, namespace, package_id);

        let mut sprite = Sprite::new(game_io, icon_texture);
        sprite.set_position(position);
        sprite_queue.draw_sprite(&sprite);
    }

    pub fn icon_texture<'a>(
        game_io: &'a GameIO,
        namespace: PackageNamespace,
        package_id: &PackageId,
    ) -> (Arc<Texture>, &'a str) {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;

        if let Some(package) = package_manager.package_or_fallback(namespace, package_id) {
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
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let package_manager = &globals.card_packages;
        let ns = PackageNamespace::Local;

        if let Some(package) = package_manager.package_or_fallback(ns, package_id) {
            let path = package.preview_texture_path.as_str();
            let texture = assets.texture(game_io, path);

            if texture.size() != UVec2::ONE {
                return (texture, path);
            }
        }

        let path = ResourcePaths::CARD_PREVIEW_MISSING;
        (assets.texture(game_io, path), path)
    }
}
