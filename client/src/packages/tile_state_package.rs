use super::{Package, PackageInfo};
use crate::bindable::Element;
use crate::render::FrameTime;
use crate::render::ui::{PackageListing, PackagePreviewData};
use packets::structures::FileHash;
use serde::Deserialize;
use std::sync::Arc;

#[derive(Deserialize, Default)]
#[serde(default)]
struct TileStateMeta {
    category: String,
    name: String,
    long_name: String,
    description: String,
    state_name: String,
    texture_path: String,
    animation_path: String,
    max_lifetime: Option<FrameTime>,
    hide_frame: bool,
    hide_body: bool,
    hole: bool,
    cleanser_element: String,
    tags: Vec<String>,
}

pub struct TileStatePackage {
    pub package_info: PackageInfo,
    pub name: Arc<str>,
    pub long_name: Arc<str>,
    pub description: Arc<str>,
    pub state_name: String,
    pub texture_path: String,
    pub animation_path: String,
    pub max_lifetime: Option<FrameTime>,
    pub hide_frame: bool,
    pub hide_body: bool,
    pub hole: bool,
    pub cleanser_element: Option<Element>,
}

impl Package for TileStatePackage {
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
            description: self.description.clone(),
            creator: Default::default(),
            hash: self.package_info.hash,
            preview_data: PackagePreviewData::TileState,
            dependencies: self.package_info.requirements.clone(),
        }
    }

    fn load_new(package_info: PackageInfo, package_table: toml::Table) -> Self {
        let mut package = Self {
            package_info,
            name: Default::default(),
            long_name: Default::default(),
            description: Default::default(),
            state_name: String::new(),
            texture_path: String::new(),
            animation_path: String::new(),
            max_lifetime: None,
            hide_frame: false,
            hide_body: false,
            hole: false,
            cleanser_element: None,
        };

        let meta: TileStateMeta = match package_table.try_into() {
            Ok(toml) => toml,
            Err(e) => {
                log::error!("Failed to parse {:?}:\n{e}", package.package_info.toml_path);
                return package;
            }
        };

        package.package_info.tags = meta.tags.into_iter().map(|s| s.into()).collect();

        if meta.category != "tile_state" {
            log::error!(
                "Missing `category = \"tile_state\"` in {:?}",
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
        package.description = meta.description.into();
        package.state_name = meta.state_name;
        package.texture_path = base_path.clone() + &meta.texture_path;
        package.animation_path = base_path.clone() + &meta.animation_path;
        package.max_lifetime = meta.max_lifetime;
        package.hide_frame = meta.hide_frame;
        package.hide_body = meta.hide_body;
        package.hole = meta.hole;
        package.cleanser_element = Some(Element::from(meta.cleanser_element));

        if package.cleanser_element == Some(Element::None) {
            package.cleanser_element = None;
        }

        package
    }
}
