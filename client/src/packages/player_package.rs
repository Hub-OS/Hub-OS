use super::*;
use crate::bindable::Element;
use crate::render::ui::{PackageListing, PackagePreviewData};
use packets::structures::{FileHash, TextureAnimPathPair};
use serde::Deserialize;
use std::sync::Arc;

#[derive(Deserialize, Default)]
#[serde(default)]
struct PlayerMeta {
    category: String,
    long_name: String,
    name: String,
    element: String,
    health: i32,
    mega_boost: i8,
    giga_boost: i8,
    dark_boost: i8,
    deck_boost: i8,
    description: String,
    long_description: String,
    preview_texture_path: String,
    overworld_animation_path: String,
    overworld_texture_path: String,
    mugshot_texture_path: String,
    mugshot_animation_path: String,
    emotions_texture_path: String,
    emotions_animation_path: String,
    tags: Vec<String>,
}

#[derive(Default, Clone)]
pub struct PlayerPackage {
    pub package_info: PackageInfo,
    pub long_name: Arc<str>,
    pub name: Arc<str>,
    pub element: Element,
    pub health: i32,
    pub mega_boost: i8,
    pub giga_boost: i8,
    pub dark_boost: i8,
    pub deck_boost: i8,
    pub description: Arc<str>,
    pub long_description: Arc<str>,
    pub icon_texture_path: String,
    pub preview_texture_path: String,
    pub overworld_paths: TextureAnimPathPair<'static>,
    pub mugshot_paths: TextureAnimPathPair<'static>,
    pub emotions_paths: Option<TextureAnimPathPair<'static>>,
}

impl Package for PlayerPackage {
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
            preview_data: PackagePreviewData::Player {
                element: self.element,
                health: self.health,
            },
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = PlayerPackage {
            package_info,
            ..Default::default()
        };

        let meta: PlayerMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        package.package_info.tags = meta.tags.into_iter().map(|s| s.into()).collect();

        if meta.category != "player" {
            log::error!(
                "Missing `category = \"player\"` in {:?}",
                package.package_info.toml_path
            );
        }

        let base_path = &package.package_info.base_path;

        package.name = meta.name.into();
        package.long_name = if meta.long_name.is_empty() {
            package.name.clone()
        } else {
            meta.long_name.into()
        };
        package.element = Element::from(meta.element);
        package.health = meta.health;
        package.mega_boost = meta.mega_boost;
        package.giga_boost = meta.giga_boost;
        package.dark_boost = meta.dark_boost;
        package.deck_boost = meta.deck_boost;
        package.description = meta.description.into();
        package.long_description = meta.long_description.into();
        package.preview_texture_path = base_path.clone() + &meta.preview_texture_path;
        package.overworld_paths.texture = (base_path.clone() + &meta.overworld_texture_path).into();
        package.overworld_paths.animation =
            (base_path.clone() + &meta.overworld_animation_path).into();
        package.mugshot_paths.texture = (base_path.clone() + &meta.mugshot_texture_path).into();
        package.mugshot_paths.animation = (base_path.clone() + &meta.mugshot_animation_path).into();

        if !meta.emotions_texture_path.is_empty() || !meta.emotions_animation_path.is_empty() {
            package.emotions_paths = Some(TextureAnimPathPair {
                texture: (base_path.clone() + &meta.emotions_texture_path).into(),
                animation: (base_path.clone() + &meta.emotions_animation_path).into(),
            });
        }

        package
    }
}
