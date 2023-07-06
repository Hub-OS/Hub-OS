use super::super::{Asset, Direction};
use super::map_layer::MapLayer;
use super::map_object::{MapObject, MapObjectData, MapObjectSpecification};
use super::Tile;
use crate::helpers::unwrap_and_parse_or_default;
use std::collections::HashMap;

#[derive(Clone)]
pub struct TilesetInfo {
    pub first_gid: u32,
    pub path: String,
}

#[derive(Clone)]
pub struct Map {
    name: String,
    background_texture_path: String,
    background_animation_path: String,
    background_vel_x: f32,
    background_vel_y: f32,
    background_parallax: f32,
    foreground_texture_path: String,
    foreground_animation_path: String,
    foreground_vel_x: f32,
    foreground_vel_y: f32,
    foreground_parallax: f32,
    music_path: String,
    custom_properties: HashMap<String, String>,
    width: usize,
    height: usize,
    tile_width: u32,
    tile_height: u32,
    spawn_x: f32,
    spawn_y: f32,
    spawn_z: f32,
    spawn_direction: Direction,
    tilesets: Vec<TilesetInfo>,
    layers: Vec<MapLayer>,
    next_layer_id: u32,
    objects: Vec<MapObject>,
    next_object_id: u32,
    asset_stale: bool,
    cached: bool,
    cached_string: String,
}

impl Map {
    pub fn from(text: &str) -> Map {
        let mut map = Map {
            name: String::new(),
            background_texture_path: String::new(),
            background_animation_path: String::new(),
            background_vel_x: 0.0,
            background_vel_y: 0.0,
            background_parallax: 0.0,
            foreground_texture_path: String::new(),
            foreground_animation_path: String::new(),
            foreground_vel_x: 0.0,
            foreground_vel_y: 0.0,
            foreground_parallax: 0.0,
            music_path: String::new(),
            custom_properties: HashMap::new(),
            width: 0,
            height: 0,
            tile_width: 0,
            tile_height: 0,
            spawn_x: 0.0,
            spawn_y: 0.0,
            spawn_z: 0.0,
            spawn_direction: Direction::None,
            tilesets: Vec::new(),
            layers: Vec::new(),
            next_layer_id: 0,
            objects: Vec::new(),
            next_object_id: 0,
            asset_stale: true,
            cached: false,
            cached_string: String::from(""),
        };

        let Ok(map_document) = roxmltree::Document::parse(text) else {
            log::error!("Invalid Tiled map file");
            return map;
        };

        let map_element = map_document.root_element();

        map.width = unwrap_and_parse_or_default(map_element.attribute("width"));
        map.height = unwrap_and_parse_or_default(map_element.attribute("height"));
        map.tile_width = unwrap_and_parse_or_default(map_element.attribute("tilewidth"));
        map.tile_height = unwrap_and_parse_or_default(map_element.attribute("tileheight"));

        map.next_layer_id = unwrap_and_parse_or_default(map_element.attribute("nextlayerid"));
        map.next_object_id = unwrap_and_parse_or_default(map_element.attribute("nextobjectid"));

        let scale_x = 1.0 / (map.tile_width as f32 * 0.5);
        let scale_y = 1.0 / map.tile_height as f32;

        let mut object_layers = 0;

        for child in map_element.children() {
            match child.tag_name().name() {
                "properties" => {
                    for property in child.children() {
                        let name = property.attribute("name").unwrap_or_default();
                        let value = property
                            .attribute("value")
                            .or_else(|| child.text())
                            .unwrap_or_default();

                        map.set_custom_property(name, value.to_string());
                    }
                }
                "tileset" => {
                    let first_gid: u32 = unwrap_and_parse_or_default(child.attribute("firstgid"));
                    let mut path = child.attribute("source").unwrap_or_default().to_string();

                    const ASSETS_RELATIVE_PATH: &str = "../assets/";

                    if path.starts_with(ASSETS_RELATIVE_PATH) {
                        path =
                            String::from("/server/assets/") + &path[ASSETS_RELATIVE_PATH.len()..];
                    }

                    map.tilesets.push(TilesetInfo { first_gid, path });
                }
                "layer" => {
                    let id: u32 = unwrap_and_parse_or_default(child.attribute("id"));
                    let name: String = child.attribute("name").unwrap_or_default().to_string();

                    // map name might be missing if the file wasn't generated
                    map.indicate_layer_offset_issues(name.as_str(), map.layers.len(), child);

                    let Some(data_element) =
                        child.children().find(|el| el.tag_name().name() == "data")
                    else {
                        log::error!("{}: Missing data element for layer {:?}!", map.name, name);
                        continue;
                    };

                    if data_element.attribute("encoding") != Some("csv") {
                        log::warn!(
                            "{}: Layer {:?} is using incorrect format, only CSV format is supported! (Check map properties)",
                            map.name, name
                        );
                        MapLayer::new(id, name, map.width, map.height, Vec::new());
                        continue;
                    }

                    // actual handling
                    let text = data_element.text().unwrap_or_default();

                    let data: Vec<u32> = text
                        .split(',')
                        .map(|value| value.trim().parse().unwrap_or_default())
                        .collect();

                    let mut layer = MapLayer::new(id, name, map.width, map.height, data);

                    let visible = child.attribute("visible").unwrap_or_default() != "0";
                    layer.set_visible(visible);

                    map.layers.push(layer);
                }
                "objectgroup" => {
                    let name: &str = child.attribute("name").unwrap_or_default();

                    // map name might be missing if the file wasn't generated
                    map.indicate_layer_offset_issues(name, object_layers, child);

                    if object_layers + 1 != map.layers.len() {
                        log::warn!("{}: Layer {:?} will link to layer {}! (Layer order starting from bottom is Tile, Object, Tile, Object, etc)", map.name, name, object_layers);
                    }

                    for object_element in child.children() {
                        let map_object =
                            MapObject::from(object_element, object_layers, scale_x, scale_y);

                        if map_object.class == "Home Warp" {
                            map.spawn_x = map_object.x + map_object.height * 0.5;
                            map.spawn_y = map_object.y + map_object.height * 0.5;
                            map.spawn_z = object_layers as f32;

                            let direction_string = map_object
                                .custom_properties
                                .get("Direction")
                                .map(|string| string.as_str())
                                .unwrap_or_default();

                            map.spawn_direction = Direction::from(direction_string);

                            // make sure direction is set if the spawn is on a home warp
                            // otherwise the player will immediately warp out
                            if matches!(map.spawn_direction, Direction::None) {
                                map.spawn_direction = Direction::UpRight;
                            }
                        }

                        map.objects.push(map_object);
                    }

                    object_layers += 1;
                }
                _ => {}
            }
        }

        if map_element.attribute("orientation") != Some("isometric") {
            log::warn!("{}: Only Isometric orientation is supported!", map.name);
        }

        if map_element.attribute("infinite") == Some("1") {
            log::warn!("{}: Infinite maps are not supported!", map.name);
        }

        if !matches!(map_element.attribute("staggerindex"), None | Some("odd")) {
            log::warn!("{}: Stagger Index must be set to Odd!", map.name);
        }

        map
    }

    fn indicate_layer_offset_issues(
        &self,
        layer_name: &str,
        layer_index: usize,
        layer_element: roxmltree::Node,
    ) {
        // warnings
        let manual_horizontal_offset: i32 =
            unwrap_and_parse_or_default(layer_element.attribute("offsetx"));
        let manual_vertical_offset: i32 =
            unwrap_and_parse_or_default(layer_element.attribute("offsety"));
        let correct_vertical_offset = layer_index as i32 * -((self.tile_height / 2) as i32);

        if manual_horizontal_offset != 0 {
            log::warn!(
                "{}: Layer {:?} has incorrect horizontal offset! (Should be 0)",
                self.name,
                layer_name
            );
        }

        if manual_vertical_offset != correct_vertical_offset {
            log::warn!(
                "{}: Layer {:?} has incorrect vertical offset! (Should be {})",
                self.name,
                layer_name,
                correct_vertical_offset
            );
        }
    }

    pub fn tilesets(&self) -> &Vec<TilesetInfo> {
        &self.tilesets
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn set_name(&mut self, name: String) {
        self.custom_properties
            .insert(String::from("Name"), name.clone());

        self.name = name;
        self.mark_dirty();
    }

    pub fn music_path(&self) -> &str {
        &self.music_path
    }

    pub fn set_music_path(&mut self, path: String) {
        self.custom_properties
            .insert(String::from("Music"), path.clone());
        self.custom_properties.remove("Song");

        self.music_path = path;
        self.mark_dirty();
    }

    pub fn background_texture_path(&self) -> &str {
        &self.background_texture_path
    }

    pub fn set_background_texture_path(&mut self, path: String) {
        self.custom_properties
            .insert(String::from("Background Texture"), path.clone());

        self.background_texture_path = path;
        self.mark_dirty();
    }

    pub fn background_animation_path(&self) -> &str {
        &self.background_animation_path
    }

    pub fn set_background_animation_path(&mut self, path: String) {
        self.custom_properties
            .insert(String::from("Background Animation"), path.clone());

        self.background_animation_path = path;
        self.mark_dirty();
    }

    pub fn background_velocity(&self) -> (f32, f32) {
        (self.background_vel_x, self.background_vel_y)
    }

    pub fn set_background_velocity(&mut self, x: f32, y: f32) {
        self.custom_properties
            .insert(String::from("Background Vel X"), x.to_string());
        self.custom_properties
            .insert(String::from("Background Vel Y"), y.to_string());

        self.background_vel_x = x;
        self.background_vel_y = y;
        self.mark_dirty();
    }

    pub fn background_parallax(&self) -> f32 {
        self.background_parallax
    }

    pub fn set_background_parallax(&mut self, parallax: f32) {
        self.custom_properties
            .insert(String::from("Background Parallax"), parallax.to_string());

        self.background_parallax = parallax;
        self.mark_dirty();
    }

    pub fn foreground_texture_path(&self) -> &str {
        &self.foreground_texture_path
    }

    pub fn set_foreground_texture_path(&mut self, path: String) {
        self.custom_properties
            .insert(String::from("Foreground Texture"), path.clone());

        self.foreground_texture_path = path;
        self.mark_dirty();
    }

    pub fn foreground_animation_path(&self) -> &str {
        &self.foreground_animation_path
    }

    pub fn set_foreground_animation_path(&mut self, path: String) {
        self.custom_properties
            .insert(String::from("Foreground Animation"), path.clone());

        self.foreground_animation_path = path;
        self.mark_dirty();
    }

    pub fn foreground_velocity(&self) -> (f32, f32) {
        (self.foreground_vel_x, self.foreground_vel_y)
    }

    pub fn set_foreground_velocity(&mut self, x: f32, y: f32) {
        self.custom_properties
            .insert(String::from("Foreground Vel X"), x.to_string());
        self.custom_properties
            .insert(String::from("Foreground Vel Y"), y.to_string());

        self.foreground_vel_x = x;
        self.foreground_vel_y = y;
        self.mark_dirty();
    }

    pub fn foreground_parallax(&self) -> f32 {
        self.foreground_parallax
    }

    pub fn set_foreground_parallax(&mut self, parallax: f32) {
        self.foreground_parallax = parallax;
        self.mark_dirty();
    }

    pub fn custom_properties(&self) -> &HashMap<String, String> {
        &self.custom_properties
    }

    pub fn get_custom_property(&self, name: &str) -> Option<&str> {
        self.custom_properties.get(name).map(String::as_str)
    }

    pub fn set_custom_property(&mut self, name: &str, value: String) {
        self.custom_properties
            .insert(name.to_string(), value.clone());

        match name {
            "Name" => {
                self.name = value;
            }
            "Background Texture" => {
                self.background_texture_path = value;
            }
            "Background Animation" => {
                self.background_animation_path = value;
            }
            "Background Vel X" => {
                self.background_vel_x = value.parse().unwrap_or_default();
            }
            "Background Vel Y" => {
                self.background_vel_y = value.parse().unwrap_or_default();
            }
            "Background Parallax" => {
                self.background_parallax = value.parse().unwrap_or_default();
            }
            "Foreground Texture" => {
                self.foreground_texture_path = value;
            }
            "Foreground Animation" => {
                self.foreground_animation_path = value;
            }
            "Foreground Vel X" => {
                self.foreground_vel_x = value.parse().unwrap_or_default();
            }
            "Foreground Vel Y" => {
                self.foreground_vel_y = value.parse().unwrap_or_default();
            }
            "Foreground Parallax" => {
                self.foreground_parallax = value.parse().unwrap_or_default();
            }
            "Music" | "Song" => {
                self.music_path = value;
            }
            _ => {}
        }

        self.mark_dirty();
    }

    pub fn width(&self) -> usize {
        self.width
    }

    pub fn height(&self) -> usize {
        self.height
    }

    pub fn layer_count(&self) -> usize {
        self.layers.len()
    }

    pub fn tile_width(&self) -> u32 {
        self.tile_width
    }

    pub fn tile_height(&self) -> u32 {
        self.tile_height
    }

    pub fn spawn_position(&self) -> (f32, f32, f32) {
        (self.spawn_x, self.spawn_y, self.spawn_z)
    }

    pub fn set_spawn_position(&mut self, x: f32, y: f32, z: f32) {
        self.spawn_x = x;
        self.spawn_y = y;
        self.spawn_z = z;
    }

    pub fn spawn_direction(&self) -> Direction {
        self.spawn_direction
    }

    pub fn set_spawn_direction(&mut self, direction: Direction) {
        self.spawn_direction = direction;
    }

    pub fn get_tile(&self, x: usize, y: usize, z: usize) -> Tile {
        let Some(layer) = self.layers.get(z) else {
            return Tile::default();
        };

        layer.get_tile(x, y)
    }

    pub fn set_tile(&mut self, x: usize, y: usize, z: usize, tile: Tile) {
        // todo: expand world instead of rejecting
        if self.width <= x || self.height <= y || self.layers.len() <= z {
            return;
        }
        let layer: &mut MapLayer = &mut self.layers[z];

        if layer.get_tile(x, y) != tile {
            layer.set_tile(x, y, tile);
            self.mark_dirty();
        }
    }

    pub fn objects(&self) -> &Vec<MapObject> {
        &self.objects
    }

    pub fn get_object_by_id(&self, id: u32) -> Option<&MapObject> {
        self.objects.iter().find(|&o| o.id == id)
    }

    pub fn get_object_by_name(&self, name: &str) -> Option<&MapObject> {
        self.objects.iter().find(|&o| o.name == name)
    }

    pub fn create_object(&mut self, specification: MapObjectSpecification) -> u32 {
        let id = self.next_object_id;

        let map_object = MapObject {
            id,
            name: specification.name,
            class: specification.class,
            x: specification.x,
            y: specification.y,
            visible: specification.visible,
            layer: specification.layer,
            width: specification.width,
            height: specification.height,
            rotation: specification.rotation,
            data: specification.data,
            custom_properties: specification.custom_properties,
        };

        self.objects.push(map_object);

        self.next_object_id += 1;
        self.mark_dirty();

        id
    }

    pub fn remove_object(&mut self, id: u32) {
        if let Some(index) = self.objects.iter().position(|object| object.id == id) {
            self.objects.remove(index);

            self.mark_dirty();
        }
    }

    pub fn set_object_name(&mut self, id: u32, name: String) {
        if let Some(object) = self.objects.iter_mut().find(|object| object.id == id) {
            object.name = name;

            self.mark_dirty();
        }
    }

    pub fn set_object_class(&mut self, id: u32, class: String) {
        if let Some(object) = self.objects.iter_mut().find(|object| object.id == id) {
            object.class = class;

            self.mark_dirty();
        }
    }

    pub fn set_object_custom_property(&mut self, id: u32, name: String, value: String) {
        if let Some(object) = self.objects.iter_mut().find(|object| object.id == id) {
            object.custom_properties.insert(name, value);

            self.mark_dirty();
        }
    }

    pub fn resize_object(&mut self, id: u32, width: f32, height: f32) {
        if let Some(object) = self.objects.iter_mut().find(|object| object.id == id) {
            if matches!(object.data, MapObjectData::Point) {
                // cant resize a point
                return;
            }

            object.width = width;
            object.height = height;

            self.mark_dirty();
        }
    }

    pub fn set_object_rotation(&mut self, id: u32, rotation: f32) {
        if let Some(object) = self.objects.iter_mut().find(|object| object.id == id) {
            object.rotation = rotation;

            self.mark_dirty();
        }
    }

    pub fn set_object_visibility(&mut self, id: u32, visibility: bool) {
        if let Some(object) = self.objects.iter_mut().find(|object| object.id == id) {
            object.visible = visibility;

            self.mark_dirty();
        }
    }

    pub fn move_object(&mut self, id: u32, x: f32, y: f32, layer: usize) {
        if let Some(object) = self.objects.iter_mut().find(|object| object.id == id) {
            object.x = x;
            object.y = y;
            object.layer = layer;

            self.mark_dirty();
        }
    }

    pub fn set_object_data(&mut self, id: u32, data: MapObjectData) {
        if let Some(object) = self.objects.iter_mut().find(|object| object.id == id) {
            object.data = data;

            self.mark_dirty();
        }
    }

    pub fn render(&mut self) -> String {
        use super::render_helpers::render_custom_properties;

        if !self.cached {
            let mut text = vec![format!(
                "\
        <?xml version=\"1.0\" encoding=\"UTF-8\"?>\
          <map version=\"1.4\" tiledversion=\"1.4.1\" orientation=\"isometric\" \
               renderorder=\"right-down\" compressionlevel=\"0\" \
               width=\"{}\" height=\"{}\" tilewidth=\"{}\" tileheight=\"{}\" \
               infinite=\"0\" nextlayerid=\"{}\" nextobjectid=\"{}\">\
            {}
        ",
                self.width,
                self.height,
                self.tile_width,
                self.tile_height,
                self.next_layer_id,
                self.next_object_id,
                render_custom_properties(&self.custom_properties)
            )];

            for tileset in &self.tilesets {
                text.push(format!(
                    "<tileset firstgid=\"{}\" source={:?}/>",
                    tileset.first_gid, tileset.path
                ));
            }

            let scale_x = 1.0 / (self.tile_width as f32 * 0.5);
            let scale_y = 1.0 / self.tile_height as f32;

            for layer_index in 0..self.layers.len() {
                text.push(self.layers[layer_index].render());

                text.push(String::from("<objectgroup>"));
                for object in &mut self.objects {
                    if object.layer >= layer_index && object.layer < layer_index + 1 {
                        text.push(object.render(scale_x, scale_y));
                    }
                }
                text.push(String::from("</objectgroup>"));
            }

            text.push(String::from("</map>"));

            self.cached_string = text.join("");
            self.cached = true;
        }

        self.cached_string.clone()
    }

    fn mark_dirty(&mut self) {
        self.asset_stale = true;
        self.cached = false;
    }

    pub(in super::super) fn asset_is_stale(&self) -> bool {
        self.asset_stale
    }

    pub fn generate_asset(&mut self) -> Asset {
        use super::super::{AssetData, AssetId};

        self.asset_stale = false;

        let tileset_paths = self.tilesets.iter().map(|tileset| &tileset.path);

        let dependencies = tileset_paths
            .chain(std::iter::once(&self.background_texture_path))
            .chain(std::iter::once(&self.background_animation_path))
            .chain(std::iter::once(&self.foreground_texture_path))
            .chain(std::iter::once(&self.foreground_animation_path))
            .chain(std::iter::once(&self.music_path))
            .filter(|path| path.starts_with("/server/")) // provided by server
            .cloned()
            .map(AssetId::AssetPath)
            .collect();

        Asset {
            data: AssetData::compress_text(self.render()),
            alternate_names: Vec::new(),
            dependencies,
            last_modified: 0,
            cachable: true,
            cache_to_disk: false,
        }
    }
}
