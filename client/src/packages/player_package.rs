use super::*;
use crate::bindable::Element;
use crate::render::ui::{PackageListing, PackagePreviewData};
use packets::structures::TextureAnimPathPair;
use serde::Deserialize;

#[derive(Deserialize, Default)]
#[serde(default)]
struct PlayerMeta {
    category: String,
    name: String,
    health: i32,
    element: String,
    description: String,
    preview_texture_path: String,
    overworld_animation_path: String,
    overworld_texture_path: String,
    mugshot_texture_path: String,
    mugshot_animation_path: String,
    emotions_texture_path: String,
    emotions_animation_path: String,
}

#[derive(Default, Clone)]
pub struct PlayerPackage {
    pub package_info: PackageInfo,
    pub name: String,
    pub health: i32,
    pub element: Element,
    pub description: String,
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

    fn create_package_listing(&self) -> PackageListing {
        PackageListing {
            id: self.package_info.id.clone(),
            name: self.name.clone(),
            description: self.description.clone(),
            creator: String::new(),
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

        if meta.category != "player" {
            log::error!(
                "Missing `category = \"player\"` in {:?}",
                package.package_info.toml_path
            );
        }

        let base_path = &package.package_info.base_path;

        package.name = meta.name;
        package.health = meta.health;
        package.element = Element::from(meta.element);
        package.description = meta.description;
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
