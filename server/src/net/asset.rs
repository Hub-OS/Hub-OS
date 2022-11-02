use packets::structures::{AssetData, PackageCategory};

#[derive(Clone, Debug)]
pub struct PackageInfo {
    pub name: String,
    pub id: String,
    pub category: PackageCategory,
}

#[derive(Clone, Debug)]
pub struct Asset {
    pub data: AssetData,
    pub alternate_names: Vec<AssetID>,
    pub dependencies: Vec<AssetID>,
    pub last_modified: u64,
    pub cachable: bool, // allows the server to know if it should update other clients with this asset, clients will cache in memory
    pub cache_to_disk: bool, // allows the client to know if they should cache this asset for rejoins or if it's dynamic
}

impl packets::structures::AssetTrait for Asset {
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
pub enum AssetID {
    AssetPath(String),
    Package(PackageInfo),
}

impl Asset {
    pub fn load_from_memory(path: &std::path::Path, data: &[u8]) -> Asset {
        let asset_data = resolve_asset_data(path, data);

        let last_modified = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .expect("Current time is before epoch?")
            .as_secs();

        let mut asset = Asset {
            data: asset_data,
            alternate_names: Vec::new(),
            dependencies: Vec::new(),
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

        let data = fs::read(&path).unwrap_or_default();
        let asset_data = resolve_asset_data(path, &data);

        let mut last_modified = 0;

        if let Ok(file_meta) = fs::metadata(&path) {
            if let Ok(time) = file_meta.modified() {
                last_modified = time
                    .duration_since(std::time::UNIX_EPOCH)
                    .unwrap_or_default()
                    .as_secs();
            }
        }

        let mut asset = Asset {
            data: asset_data,
            alternate_names: Vec::new(),
            dependencies: Vec::new(),
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
            "zip" => self.resolve_zip_dependencies(path),
            _ => {}
        }
    }

    fn resolve_tsx_dependencies(&mut self) {
        let data = if let AssetData::Text(data) = &self.data {
            data
        } else {
            return;
        };

        if let Ok(tileset_element) = data.parse::<minidom::Element>() {
            for child in tileset_element.children() {
                match child.name() {
                    "image" => {
                        let source = child.attr("source").unwrap_or_default();

                        if source.starts_with("/server/") {
                            self.dependencies
                                .push(AssetID::AssetPath(source.to_string()))
                        }
                    }
                    "tile" => {
                        self.resolve_tile_dependencies(child);
                    }
                    _ => {}
                }
            }
        }
    }

    fn resolve_tile_dependencies(&mut self, tile_element: &minidom::Element) {
        let tile_class_option = tile_element
            .attr("class")
            .or_else(|| tile_element.attr("type"));

        let tile_class = if let Some(tile_class) = tile_class_option {
            tile_class
        } else {
            return;
        };

        let properties_element = if let Some(properties_element) =
            tile_element.get_child("properties", minidom::NSChoice::Any)
        {
            properties_element
        } else {
            return;
        };

        for property_element in properties_element.children() {
            #[allow(clippy::single_match)]
            match (tile_class, property_element.attr("name")) {
                ("Conveyor" | "Ice", Some("Sound Effect")) => {
                    let value = property_element.attr("value").unwrap_or_default();

                    if value.starts_with("/server/") {
                        self.dependencies
                            .push(AssetID::AssetPath(value.to_string()));
                    }
                }
                _ => {}
            }
        }
    }

    fn resolve_zip_dependencies(&mut self, path: &std::path::Path) {
        let data = if let AssetData::Data(data) = &self.data {
            data
        } else {
            return;
        };

        use std::io::Cursor;
        use zip::ZipArchive;

        let data_cursor = Cursor::new(data);

        let mut archive = if let Ok(archive) = ZipArchive::new(data_cursor) {
            archive
        } else {
            return;
        };

        if let Ok(mut encounter_file) = archive.by_name("entry.lua") {
            use std::io::Read;

            let mut entry_script = String::new();

            if encounter_file.read_to_string(&mut entry_script).is_ok() {
                Asset::resolve_package_dependencies(
                    &mut self.alternate_names,
                    &mut self.dependencies,
                    path,
                    entry_script,
                );
            }
        };
    }

    fn resolve_package_dependencies(
        alternate_names: &mut Vec<AssetID>,
        dependencies: &mut Vec<AssetID>,
        path: &std::path::Path,
        entry_script: String,
    ) {
        use closure::closure;
        use std::cell::RefCell;

        let lua_ctx = mlua::Lua::new();

        let alternate_names = RefCell::new(alternate_names);
        let dependencies = RefCell::new(dependencies);
        let package_info = RefCell::new(PackageInfo {
            name: String::new(),
            id: String::new(),
            category: PackageCategory::Library,
        });

        let result = (|| -> mlua::Result<()> {
            lua_ctx.scope(|scope| -> mlua::Result<()> {
                let globals = lua_ctx.globals();

                globals.set("_card_props", lua_ctx.create_table()?)?;

                let engine_table = lua_ctx.create_table()?;

                // subpackage resolution
                let create_define_func = |category: PackageCategory| {
                    scope.create_function_mut(
                        closure!(move category, ref alternate_names, |_, id: String| {
                          let package_info = PackageInfo{ name: String::new(), id, category };

                          alternate_names
                            .borrow_mut()
                            .push(AssetID::Package(package_info));

                          Ok(())
                        }),
                    )
                };

                engine_table.set("define_card", create_define_func(PackageCategory::Card)?)?;
                engine_table.set(
                    "define_character",
                    create_define_func(PackageCategory::Character)?,
                )?;
                engine_table.set(
                    "define_library",
                    create_define_func(PackageCategory::Library)?,
                )?;

                // dependency resolution
                let create_require_func = |category: PackageCategory| {
                    scope.create_function_mut(
                        closure!(move category, ref dependencies, |_, id: String| {
                          let package_info = PackageInfo{ name: String::new(), id, category };

                          dependencies.borrow_mut().push(AssetID::Package(package_info));

                          Ok(())
                        }),
                    )
                };

                engine_table.set("requires_card", create_require_func(PackageCategory::Card)?)?;
                engine_table.set(
                    "requires_character",
                    create_require_func(PackageCategory::Character)?,
                )?;
                engine_table.set(
                    "requires_library",
                    create_require_func(PackageCategory::Library)?,
                )?;

                // id resolution
                let package_table = lua_ctx.create_table()?;

                package_table.set(
                    "declare_package_id",
                    scope.create_function_mut(|_, (_, id): (mlua::Table, String)| {
                        package_info.borrow_mut().id = id;
                        Ok(())
                    })?,
                )?;

                // name resolution
                package_table.set(
                    "set_name",
                    scope.create_function_mut(|_, (_, name): (mlua::Table, String)| {
                        package_info.borrow_mut().name = name;
                        Ok(())
                    })?,
                )?;

                // resolving category
                let create_category_stub = |category: PackageCategory| {
                    scope.create_function_mut(
                        closure!(move category, ref package_info, |_, _: mlua::MultiValue| {
                          package_info.borrow_mut().category = category;

                          Ok(())
                        }),
                    )
                };

                package_table.set("set_mutator", create_category_stub(PackageCategory::Block)?)?;

                package_table.set("set_codes", create_category_stub(PackageCategory::Card)?)?;
                package_table.set(
                    "get_card_props",
                    scope.create_function_mut(|lua_ctx, _: mlua::Table| {
                        package_info.borrow_mut().category = PackageCategory::Card;

                        let card_props: mlua::Table = lua_ctx.globals().get("_card_props")?;

                        Ok(card_props)
                    })?,
                )?;

                package_table.set(
                    "set_overworld_animation_path",
                    create_category_stub(PackageCategory::Player)?,
                )?;
                package_table.set(
                    "set_overworld_texture_path",
                    create_category_stub(PackageCategory::Player)?,
                )?;
                package_table.set(
                    "set_mugshot_texture_path",
                    create_category_stub(PackageCategory::Player)?,
                )?;
                package_table.set(
                    "set_mugshot_animation_path",
                    create_category_stub(PackageCategory::Player)?,
                )?;

                // stubs
                let create_nil_stub = || scope.create_function_mut(|_, _: mlua::MultiValue| Ok(()));
                let create_table_stub = || {
                    scope.create_function_mut(|lua_ctx, _: mlua::MultiValue| lua_ctx.create_table())
                };

                globals.set("_game_folder_path", "")?;
                globals.set("_folder_path", "")?;
                globals.set("include", create_nil_stub()?)?;

                globals.set("Blocks", lua_ctx.create_table()?)?;
                globals.set("Element", lua_ctx.create_table()?)?;
                globals.set("CardClass", lua_ctx.create_table()?)?;
                globals.set("TileState", lua_ctx.create_table()?)?;
                globals.set("Team", lua_ctx.create_table()?)?;
                globals.set("Rank", lua_ctx.create_table()?)?;

                let color_table = lua_ctx.create_table()?;
                color_table.set("new", create_table_stub()?)?;
                globals.set("Color", color_table)?;

                engine_table.set("load_texture", create_nil_stub()?)?;
                engine_table.set("load_audio", create_nil_stub()?)?;
                globals.set("Engine", engine_table)?;

                package_table.set("set_description", create_nil_stub()?)?;
                package_table.set("set_color", create_nil_stub()?)?;
                package_table.set("set_shape", create_nil_stub()?)?;
                package_table.set("as_program", create_nil_stub()?)?;
                package_table.set("set_special_description", create_nil_stub()?)?;
                package_table.set("set_preview_texture_path", create_nil_stub()?)?;
                package_table.set("set_icon_texture_path", create_nil_stub()?)?;
                package_table.set("set_speed", create_nil_stub()?)?;
                package_table.set("set_attack", create_nil_stub()?)?;
                package_table.set("set_health", create_nil_stub()?)?;
                package_table.set("set_charged_attack", create_nil_stub()?)?;

                // execution

                lua_ctx.load(&entry_script).exec()?;

                if let Ok(requires_scripts_func) =
                    globals.get::<&str, mlua::Function>("package_requires_scripts")
                {
                    requires_scripts_func.call(())?;
                }

                let init_func: mlua::Function = globals.get("package_init")?;
                init_func.call(package_table)?;

                // encounter detection
                let package_build_func: mlua::Value = globals.get("package_build")?;

                if let mlua::Value::Function(_) = package_build_func {
                    package_info.borrow_mut().category = PackageCategory::Battle;
                }

                // name resolution
                let card_props: mlua::Table = lua_ctx.globals().get("_card_props")?;

                if let Ok(card_name) = card_props.get("shortname") {
                    package_info.borrow_mut().name = card_name;
                }

                Ok(())
            })?;

            // prepend the alternate name to make the first result when resolving the category
            alternate_names
                .into_inner()
                .insert(0, AssetID::Package(package_info.into_inner()));

            Ok(())
        })();

        if let Err(e) = result {
            log::error!(
                "Failed to load \"entry.lua\" in {:?}:\n{}",
                path.display(),
                e
            );
        }
    }

    pub fn resolve_package_info(&self) -> Option<&PackageInfo> {
        self.alternate_names
            .iter()
            .find_map(|asset_id| match asset_id {
                AssetID::Package(info) => Some(info),
                _ => None,
            })
    }
}

pub fn get_player_texture_path(player_id: &str) -> String {
    String::from("/server/players/") + player_id + ".texture"
}

pub fn get_player_animation_path(player_id: &str) -> String {
    String::from("/server/players/") + player_id + ".animation"
}

pub fn get_player_mugshot_texture_path(player_id: &str) -> String {
    String::from("/server/players/") + player_id + "_mug.texture"
}

pub fn get_player_mugshot_animation_path(player_id: &str) -> String {
    String::from("/server/players/") + player_id + "_mug.animation"
}

pub fn get_map_path(map_id: &str) -> String {
    String::from("/server/maps/") + map_id + ".tmx"
}

fn resolve_asset_data(path: &std::path::Path, data: &[u8]) -> AssetData {
    let extension = path
        .extension()
        .unwrap_or_default()
        .to_str()
        .unwrap_or_default();

    match extension.to_lowercase().as_str() {
        "png" | "bmp" => AssetData::Texture(data.to_vec()),
        "flac" | "mp3" | "wav" | "mid" | "midi" | "ogg" => AssetData::Audio(data.to_vec()),
        "zip" => AssetData::Data(data.to_vec()),
        "tsx" => {
            let original_data = String::from_utf8_lossy(data);
            let translated_data = translate_tsx(path, &original_data);

            if translated_data == None {
                log::warn!("Invalid .tsx file: {:?}", path);
            }

            AssetData::Text(translated_data.unwrap_or_else(|| original_data.to_string()))
        }
        _ => AssetData::Text(String::from_utf8_lossy(data).to_string()),
    }
}

fn translate_tsx(path: &std::path::Path, data: &str) -> Option<String> {
    use crate::helpers::normalize_path;

    let root_path = std::path::Path::new("/server");
    let path_base = path.parent()?;
    let mut tileset_element = data.parse::<minidom::Element>().ok()?;

    for child in tileset_element.children_mut() {
        if child.name() != "image" {
            continue;
        }

        let source = path_base.join(child.attr("source")?);
        let mut normalized_source = normalize_path(&source);

        if normalized_source.starts_with("assets") {
            // path did not escape server folders
            normalized_source = root_path.join(normalized_source);
        }

        // adjust windows paths
        let corrected_source = normalized_source.to_string_lossy().replace('\\', "/");

        child.set_attr("source", corrected_source);
    }

    let mut output: Vec<u8> = Vec::new();

    tileset_element.write_to(&mut output).ok()?;

    Some(String::from_utf8_lossy(&output[..]).into_owned())
}
