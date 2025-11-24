use packets::structures::{ActorId, AssetData, FileHash, PackageCategory, PackageId};

#[derive(Clone, Debug)]
pub struct PackageInfo {
    pub name: String,
    pub id: PackageId,
    pub category: PackageCategory,
}

#[derive(Clone, Debug)]
pub struct Asset {
    pub data: AssetData,
    pub alternate_names: Vec<AssetId>,
    pub dependencies: Vec<AssetId>,
    pub hash: FileHash,
    pub last_modified: u64,
    pub cachable: bool, // allows the server to know if it should update other clients with this asset, clients will cache in memory
    pub cache_to_disk: bool, // allows the client to know if they should cache this asset for rejoins or if it's dynamic
}

impl packets::structures::AssetTrait for Asset {
    fn hash(&self) -> FileHash {
        self.hash
    }

    fn last_modified(&self) -> u64 {
        self.last_modified
    }

    fn cache_to_disk(&self) -> bool {
        self.cache_to_disk
    }

    fn data(&self) -> &AssetData {
        &self.data
    }
}

#[derive(Clone, Debug)]
pub enum AssetId {
    AssetPath(String),
    Package(PackageInfo),
}

impl Asset {
    pub fn load_from_memory(path: &std::path::Path, data: Vec<u8>) -> Asset {
        let hash = FileHash::hash(&data);
        let asset_data = resolve_asset_data(path, data);

        let last_modified = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .expect("Current time is before epoch?")
            .as_secs();

        let mut asset = Asset {
            data: asset_data,
            alternate_names: Vec::new(),
            dependencies: Vec::new(),
            hash,
            last_modified,
            cachable: true,
            cache_to_disk: true,
        };

        asset.resolve_dependencies(path);

        if let AssetData::Text(text) = asset.data {
            asset.data = AssetData::compress_text(text)
        }

        asset
    }

    pub fn load_from_file(path: &std::path::Path) -> Asset {
        use std::fs;

        let data = fs::read(path).unwrap_or_default();
        let hash = FileHash::hash(&data);
        let asset_data = resolve_asset_data(path, data);

        let mut last_modified = 0;

        if let Ok(file_meta) = fs::metadata(path)
            && let Ok(time) = file_meta.modified()
        {
            last_modified = time
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs();
        }

        let mut asset = Asset {
            data: asset_data,
            alternate_names: Vec::new(),
            dependencies: Vec::new(),
            hash,
            last_modified,
            cachable: true,
            cache_to_disk: true,
        };

        asset.resolve_dependencies(path);

        if let AssetData::Text(text) = asset.data {
            asset.data = AssetData::compress_text(text)
        }

        asset
    }

    pub fn len(&self) -> usize {
        match &self.data {
            AssetData::Text(data) => data.len(),
            AssetData::CompressedText(data) => data.len(),
            AssetData::Texture(data) => data.len(),
            AssetData::Audio(data) => data.len(),
            AssetData::Data(data) => data.len(),
        }
    }

    // Resolves dependencies and alternate name. `load_from_*` functions automatically call this
    fn resolve_dependencies(&mut self, path: &std::path::Path) {
        let extension = path
            .extension()
            .unwrap_or_default()
            .to_str()
            .unwrap_or_default();

        match extension {
            "tsx" => self.resolve_tsx_dependencies(),
            "zip" => self.resolve_zip_meta(),
            _ => {}
        }
    }

    fn resolve_tsx_dependencies(&mut self) {
        let data = if let AssetData::Text(data) = &self.data {
            data.clone()
        } else {
            return;
        };

        if let Ok(tileset_document) = roxmltree::Document::parse(&data) {
            let tileset_element = tileset_document.root_element();
            for child in tileset_element.children() {
                match child.tag_name().name() {
                    "image" => {
                        let source = child.attribute("source").unwrap_or_default();

                        if source.starts_with("/server/") {
                            self.dependencies
                                .push(AssetId::AssetPath(source.to_string()))
                        }
                    }
                    "tile" => {
                        self.resolve_tile_dependencies(&child);
                    }
                    _ => {}
                }
            }
        }
    }

    fn resolve_tile_dependencies(&mut self, tile_element: &roxmltree::Node) {
        let tile_class_option = tile_element
            .attribute("class")
            .or_else(|| tile_element.attribute("type"));

        let Some(tile_class) = tile_class_option else {
            return;
        };

        let Some(properties_element) = tile_element
            .children()
            .find(|child| child.tag_name().name() == "properties")
        else {
            return;
        };

        for property_element in properties_element.children() {
            #[allow(clippy::single_match)]
            match (tile_class, property_element.attribute("name")) {
                ("Conveyor" | "Ice", Some("Sound Effect")) => {
                    let value = property_element.attribute("value").unwrap_or_default();

                    if value.starts_with("/server/") {
                        self.dependencies
                            .push(AssetId::AssetPath(value.to_string()));
                    }
                }
                _ => {}
            }
        }
    }

    fn resolve_zip_meta(&mut self) {
        let AssetData::Data(data) = &self.data else {
            return;
        };

        let Some(meta_text) = Self::load_package_meta(data) else {
            return;
        };

        let Ok(meta_table) = meta_text.parse::<toml::Table>() else {
            return;
        };

        let Some(package_info) = Self::resolve_package_info(&meta_table) else {
            return;
        };

        self.alternate_names.push(AssetId::Package(package_info));

        if let Some(ids) = Self::resolve_package_defines(&meta_table) {
            self.alternate_names.extend(ids);
        }

        if let Some(ids) = Self::resolve_package_dependencies(&meta_table) {
            self.dependencies.extend(ids);
        };
    }

    fn load_package_meta(zip_bytes: &[u8]) -> Option<String> {
        use std::io::Cursor;
        use std::io::Read;
        use zip::ZipArchive;

        let data_cursor = Cursor::new(zip_bytes);

        let mut archive = ZipArchive::new(data_cursor).ok()?;
        let mut meta_file = archive.by_name("package.toml").ok()?;

        let mut meta_text = String::new();
        meta_file.read_to_string(&mut meta_text).ok()?;

        Some(meta_text)
    }

    fn resolve_package_info(meta_table: &toml::Table) -> Option<PackageInfo> {
        let package = meta_table.get("package")?;

        let id = get_str(package, "id");
        let name = get_str(package, "name");
        let category = get_str(package, "category");

        Some(PackageInfo {
            name: name.to_string(),
            id: id.into(),
            category: category.into(),
        })
    }

    fn resolve_package_defines(
        meta_table: &toml::Table,
    ) -> Option<impl Iterator<Item = AssetId> + '_> {
        let defines = meta_table.get("defines")?;

        let characters = defines.get("characters")?.as_array()?;

        Some(
            characters
                .iter()
                .map(|define| PackageInfo {
                    name: get_str(define, "name").to_string(),
                    id: get_str(define, "id").into(),
                    category: PackageCategory::Character,
                })
                .map(AssetId::Package),
        )
    }

    fn resolve_package_dependencies(
        meta_table: &toml::Table,
    ) -> Option<impl Iterator<Item = AssetId> + '_> {
        let dependencies = meta_table.get("dependencies")?;

        let char_iter = Self::resolve_dependency_category(
            dependencies,
            "characters",
            PackageCategory::Character,
        );

        let aug_iter =
            Self::resolve_dependency_category(dependencies, "augments", PackageCategory::Augment);

        let battles_iter = Self::resolve_dependency_category(
            dependencies,
            "encounters",
            PackageCategory::Encounter,
        );

        let card_iter =
            Self::resolve_dependency_category(dependencies, "cards", PackageCategory::Card);

        let lib_iter =
            Self::resolve_dependency_category(dependencies, "libraries", PackageCategory::Library);

        let status_iter =
            Self::resolve_dependency_category(dependencies, "statuses", PackageCategory::Status);

        let tile_state_iter = Self::resolve_dependency_category(
            dependencies,
            "tile_states",
            PackageCategory::TileState,
        );

        Some(
            char_iter
                .chain(aug_iter)
                .chain(battles_iter)
                .chain(card_iter)
                .chain(lib_iter)
                .chain(status_iter)
                .chain(tile_state_iter),
        )
    }

    fn resolve_dependency_category<'a>(
        dependencies: &'a toml::Value,
        key: &str,
        category: PackageCategory,
    ) -> impl Iterator<Item = AssetId> + 'a {
        dependencies
            .get(key)
            .map(|value| value.as_array())
            .into_iter()
            .flatten()
            .flatten()
            .flat_map(|id| id.as_str())
            .map(move |id| PackageInfo {
                name: String::new(),
                id: id.into(),
                category,
            })
            .map(AssetId::Package)
    }

    pub fn package_info(&self) -> Option<&PackageInfo> {
        self.alternate_names
            .iter()
            .find_map(|asset_id| match asset_id {
                AssetId::Package(info) => Some(info),
                _ => None,
            })
    }
}

fn get_str<'a>(table: &'a toml::Value, key: &str) -> &'a str {
    table
        .get(key)
        .map(|v| v.as_str().unwrap_or_default())
        .unwrap_or_default()
}

pub fn get_player_texture_path(player_id: ActorId) -> String {
    format!("/server/players/{player_id:?}.texture")
}

pub fn get_player_animation_path(player_id: ActorId) -> String {
    format!("/server/players/{player_id:?}.animation")
}

pub fn get_player_mugshot_texture_path(player_id: ActorId) -> String {
    format!("/server/players/{player_id:?}_mug.texture")
}

pub fn get_player_mugshot_animation_path(player_id: ActorId) -> String {
    format!("/server/players/{player_id:?}_mug.animation")
}

pub fn get_map_path(map_id: &str) -> String {
    String::from("/server/maps/") + map_id + ".tmx"
}

fn resolve_asset_data(path: &std::path::Path, data: Vec<u8>) -> AssetData {
    let extension = path
        .extension()
        .unwrap_or_default()
        .to_str()
        .unwrap_or_default();

    match extension.to_lowercase().as_str() {
        "png" | "bmp" => AssetData::Texture(data),
        "flac" | "mp3" | "wav" | "mid" | "midi" | "ogg" => AssetData::Audio(data),
        "zip" => AssetData::Data(data),
        "tsx" => {
            let original_data = String::from_utf8_lossy(&data);
            let translated_data = translate_tsx(path, &original_data);

            if translated_data.is_none() {
                log::warn!("Invalid .tsx file: {path:?}");
            }

            AssetData::Text(translated_data.unwrap_or_else(|| original_data.to_string()))
        }
        _ => AssetData::Text(String::from_utf8_lossy(&data).to_string()),
    }
}

fn translate_tsx(path: &std::path::Path, data: &str) -> Option<String> {
    use crate::helpers::normalize_path;
    use std::ops::Range;

    let root_path = std::path::Path::new("/server");
    let path_base = path.parent()?;
    let tileset_document = roxmltree::Document::parse(data).ok()?;
    let tileset_element = tileset_document.root_element();

    let start_offset = data.as_ptr() as usize;
    let mut patches = Vec::new();

    for child in tileset_element.children() {
        if child.tag_name().name() != "image" {
            continue;
        }

        let Some(original_source) = child.attribute("source") else {
            continue;
        };

        // resolve range
        let range_start = original_source.as_ptr() as usize - start_offset;
        let range = Range {
            start: range_start,
            end: original_source.len() + range_start,
        };

        // create replacement text
        let source = path_base.join(original_source);
        let mut normalized_source = normalize_path(&source);

        if normalized_source.starts_with("assets") {
            // path did not escape server folders
            normalized_source = root_path.join(normalized_source);
        }

        // adjust windows paths
        let corrected_source = normalized_source.to_string_lossy().replace('\\', "/");

        patches.push((range, corrected_source));
    }

    // apply patches
    let mut updated_data = data.to_string();

    for (range, text) in patches.into_iter().rev() {
        updated_data.replace_range(range, &text);
    }

    Some(updated_data)
}
