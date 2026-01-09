use super::objects::shape_from_xml;
use super::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::{GameIO, Rect, Vec2};
use std::rc::Rc;
use structures::parse_util::parse_or_default;
use structures::shapes::Projection;

pub fn load_map<A: AssetManager>(game_io: &GameIO, assets: &A, data: &str) -> Option<Map> {
    let doc = match roxmltree::Document::parse(data) {
        Ok(doc) => doc,
        Err(err) => {
            log::error!("Failed to load map: {err}");
            return None;
        }
    };

    let map_element = doc.descendants().find(|n| n.tag_name().name() == "map")?;

    // organize elements
    let mut properties_element = None;
    let mut layer_elements = Vec::new();
    let mut object_layer_elements = Vec::new();
    let mut tileset_elements = Vec::new();

    for child in map_element.children() {
        match child.tag_name().name() {
            "layer" => layer_elements.push(child),
            "objectgroup" => object_layer_elements.push(child),
            "properties" => {
                properties_element = Some(child);
            }
            "tileset" => {
                tileset_elements.push(child);
            }
            _ => {}
        }
    }

    // begin building map
    let mut map = Map::new(
        parse_or_default(map_element.attribute("width")),
        parse_or_default(map_element.attribute("height")),
        parse_or_default(map_element.attribute("tilewidth")),
        parse_or_default(map_element.attribute("tileheight")),
    );

    // read custom properties
    if let Some(properties_element) = properties_element {
        for property_element in properties_element.children() {
            let property_name = property_element.attribute("name").unwrap_or_default();
            let property_value = property_element.attribute("value").unwrap_or_default();

            match property_name.to_lowercase().as_str() {
                "background texture" => {
                    map.background_properties_mut().texture_path = property_value.to_string();
                }
                "background animation" => {
                    map.background_properties_mut().animation_path = property_value.to_string();
                }
                "background vel x" => {
                    let bg_properties = map.background_properties_mut();
                    let velocity = bg_properties.velocity.get_or_insert(Default::default());
                    velocity.x = property_value.parse().unwrap_or_default();
                }
                "background vel y" => {
                    let bg_properties = map.background_properties_mut();
                    let velocity = bg_properties.velocity.get_or_insert(Default::default());
                    velocity.y = property_value.parse().unwrap_or_default();
                }
                "background parallax" => {
                    let parallax = property_value.parse().unwrap_or_default();
                    map.background_properties_mut().parallax = parallax;
                }
                "foreground texture" => {
                    map.foreground_properties_mut().texture_path = property_value.to_string();
                }
                "foreground animation" => {
                    map.foreground_properties_mut().animation_path = property_value.to_string();
                }
                "foreground vel x" => {
                    let fg_properties = map.foreground_properties_mut();
                    let velocity = fg_properties.velocity.get_or_insert(Default::default());
                    velocity.x = property_value.parse().unwrap_or_default();
                }
                "foreground vel y" => {
                    let fg_properties = map.foreground_properties_mut();
                    let velocity = fg_properties.velocity.get_or_insert(Default::default());
                    velocity.y = property_value.parse().unwrap_or_default();
                }
                "foreground parallax" => {
                    let parallax = property_value.parse().unwrap_or_default();
                    map.background_properties_mut().parallax = parallax;
                }
                "name" => {
                    map.set_name(property_value.to_string());
                }
                "music" | "song" => {
                    map.set_music_path(property_value.to_string());
                }
                _ => {
                    // println!("{}", property_name.to_lowercase());
                }
            }
        }
    }

    // load tilesets
    for map_tileset_element in tileset_elements {
        let first_gid: u32 = parse_or_default(map_tileset_element.attribute("firstgid"));
        let mut source = map_tileset_element
            .attribute("source")
            .unwrap_or_default()
            .to_string();

        if !source.starts_with("/server") {
            // client path
            // todo: hardcoded path oof, this will only be fine if all of our tiles are in this folder
            source = "resources/scenes/overworld/tiles/".to_string()
                + source.split('/').next_back().unwrap();
        }

        let document_text = assets.text(&source);
        let tileset_document = match roxmltree::Document::parse(&document_text) {
            Ok(element) => element,
            Err(err) => {
                log::error!("Failed to load {source:?}, {err}");
                continue;
            }
        };

        let tileset_element = tileset_document.root_element();

        let tileset = parse_tileset(game_io, assets, &tileset_element, first_gid);
        let tile_metas = parse_tile_metas(&tileset_element, tileset);

        for tile_meta in tile_metas {
            map.add_tile_meta(tile_meta);
        }
    }

    let layer_count = layer_elements.len().max(object_layer_elements.len());

    // build layers
    for i in 0..layer_count {
        map.add_layer();

        // add tiles to layer
        if let Some(layer_element) = layer_elements.get(i) {
            let layer = map.tile_layer_mut(i).unwrap();

            layer.set_visible(layer_element.attribute("visible") != Some("0"));

            let data_element = match layer_element
                .children()
                .find(|n| n.tag_name().name() == "data")
            {
                Some(el) => el,
                None => {
                    log::warn!("Map layer missing data element!");
                    continue;
                }
            };

            let data_text = data_element.text().unwrap_or_default().trim();

            for (row, line) in data_text.lines().enumerate() {
                for (col, tile_id_str) in line.split(',').enumerate() {
                    let tile_id = tile_id_str.parse().unwrap_or_default();

                    layer.set_tile((col as i32, row as i32).into(), Tile::new(tile_id));
                }
            }
        }

        // add objects to layer
        if let Some(object_layer_element) = object_layer_elements.get(i) {
            for child in object_layer_element.children() {
                if child.tag_name().name() != "object" {
                    continue;
                }

                if child.attribute("gid").is_some() {
                    let tile_object = TileObject::from(child);
                    map.insert_tile_object(game_io, tile_object, i);
                } else {
                    let shape_object = ShapeObject::from(child);
                    map.insert_shape_object(shape_object, i);
                }
            }
        }
    }

    Some(map)
}

fn parse_tileset<A: AssetManager>(
    game_io: &GameIO,
    assets: &A,
    tileset_element: &roxmltree::Node,
    first_gid: u32,
) -> Rc<Tileset> {
    let tile_count: u32 = parse_or_default(tileset_element.attribute("tilecount"));
    let tile_width: i32 = parse_or_default(tileset_element.attribute("tilewidth"));
    let tile_height: i32 = parse_or_default(tileset_element.attribute("tileheight"));
    let columns: u32 = parse_or_default::<u32>(tileset_element.attribute("columns")).max(1);

    let mut drawing_offset = Vec2::ZERO;
    let mut texture_path = String::new();
    let mut orientation = Projection::Orthographic;
    let mut custom_properties = CustomProperties::new();

    let mut tile_elements: Vec<Option<roxmltree::Node>> = Vec::new();
    tile_elements.resize_with(tile_count as usize, || None);

    for child in tileset_element.children() {
        match child.tag_name().name() {
            "tileoffset" => {
                drawing_offset.x = parse_or_default(child.attribute("x"));
                drawing_offset.y = parse_or_default(child.attribute("y"));
            }
            "grid" => {
                orientation = if child.attribute("orientation") == Some("isometric") {
                    Projection::Isometric
                } else {
                    Projection::Orthographic
                };
            }
            "image" => {
                let source = child.attribute("source").unwrap_or_default();

                texture_path = if !source.starts_with("/server") {
                    // client path
                    // todo: check relative to the tileset's parent with pathbuf
                    "resources/scenes/overworld/tiles/".to_string() + source
                } else {
                    source.to_string()
                };
            }
            "tile" => {
                let tile_id: u32 = parse_or_default(child.attribute("id"));

                if let Some(element) = tile_elements.get_mut(tile_id as usize) {
                    *element = Some(child);
                }
            }
            "properties" => {
                custom_properties = CustomProperties::from(child);
            }
            _ => {}
        }
    }

    let mut animator = Animator::new();

    let object_alignment = tileset_element
        .attribute("objectalignment")
        .unwrap_or_default();

    let alignment_offset = match object_alignment {
        "top" => (-tile_width / 2, 0),
        "topleft" => (0, 0),
        "topright" => (-tile_width, 0),
        "center" => (-tile_width / 2, -tile_height / 2),
        "left" => (0, -tile_height / 2),
        "right" => (-tile_width, -tile_height / 2),
        "bottomleft" => (0, -tile_height),
        "bottomright" => (-tile_width, -tile_height),
        // default to bottom
        _ => (-tile_width / 2, -tile_height),
    };

    let origin = Vec2::new((tile_width / 2) as f32, (tile_height / 2) as f32);

    for (i, tile_element) in tile_elements.into_iter().enumerate() {
        let i = i as u32;

        let animation_element = tile_element.and_then(|element| {
            element
                .children()
                .find(|child| child.tag_name().name() == "animation")
        });

        let frames = if let Some(animation_element) = animation_element {
            animation_element
                .children()
                .filter(|child| child.tag_name().name() == "frame")
                .map(|frame_element| {
                    let tile_id: u32 = parse_or_default(frame_element.attribute("tileid"));
                    let duration = (parse_or_default::<f32>(frame_element.attribute("duration"))
                        / 1000.0
                        * 60.0)
                        .round() as FrameTime;

                    let col = tile_id % columns;
                    let row = tile_id / columns;

                    let x = col * tile_width as u32;
                    let y = row * tile_height as u32;

                    AnimationFrame {
                        duration,
                        bounds: Rect::new(
                            x as f32,
                            y as f32,
                            tile_width as f32,
                            tile_height as f32,
                        ),
                        origin,
                        points: Default::default(),
                        valid: true,
                    }
                })
                .collect()
        } else {
            let col = i % columns;
            let row = i / columns;

            let x = col * tile_width as u32;
            let y = row * tile_height as u32;

            vec![AnimationFrame {
                duration: 0,
                bounds: Rect::new(x as f32, y as f32, tile_width as f32, tile_height as f32),
                origin,
                points: Default::default(),
                valid: true,
            }]
        };

        animator.add_state(i.to_string(), frames.into());
    }

    let tileset = Tileset {
        name: tileset_element
            .attribute("name")
            .unwrap_or_default()
            .to_string(),
        first_gid,
        tile_count,
        drawing_offset,
        alignment_offset: Vec2::new(alignment_offset.0 as f32, alignment_offset.1 as f32),
        orientation,
        custom_properties,
        texture: assets.texture(game_io, &texture_path),
        animator,
    };

    Rc::new(tileset)
}

fn parse_tile_metas(tileset_element: &roxmltree::Node, tileset: Rc<Tileset>) -> Vec<TileMeta> {
    let tile_count: u32 = parse_or_default(tileset_element.attribute("tilecount"));

    let mut tile_element_slots: Vec<Option<roxmltree::Node>> = Vec::new();
    tile_element_slots.resize_with(tile_count as usize, || None);

    let tile_elements = tileset_element
        .children()
        .filter(|child| child.tag_name().name() == "tile");

    for tile_element in tile_elements {
        let tile_id: usize = parse_or_default(tile_element.attribute("id"));

        if let Some(element) = tile_element_slots.get_mut(tile_id) {
            *element = Some(tile_element);
        }
    }

    let mut tile_metas: Vec<TileMeta> = Vec::new();

    for (tile_id, tile_element) in tile_element_slots.into_iter().enumerate() {
        let mut collision_shapes = Vec::new();
        let mut custom_properties = CustomProperties::new();
        let mut tile_class = TileClass::Undefined;

        if let Some(tile_element) = tile_element {
            for child in tile_element.children() {
                match child.tag_name().name() {
                    "properties" => {
                        custom_properties = CustomProperties::from(child);
                        continue;
                    }
                    "objectgroup" => {}
                    _ => continue,
                }

                // objectgroup
                for object_element in child.children() {
                    if let Some(shape) = shape_from_xml(object_element) {
                        collision_shapes.push(shape);
                    }
                }
            }

            let class = tile_element
                .attribute("class")
                .or_else(|| tile_element.attribute("type"))
                .unwrap_or_default();

            tile_class = TileClass::from(class);
        }

        let mut tile_meta = TileMeta::new(tileset.clone(), tile_id as u32);

        let get_or_fallback = |key| {
            let direct_value = custom_properties.get(key);

            if direct_value.is_empty() {
                tileset.custom_properties.get(key)
            } else {
                direct_value
            }
        };

        tile_meta.drawing_offset = tileset.drawing_offset;
        tile_meta.alignment_offset = tileset.alignment_offset;
        tile_meta.tile_class = tile_class;
        tile_meta.minimap = get_or_fallback("minimap") != "false";
        tile_meta.shadow = TileShadow::from(get_or_fallback("shadow"));
        tile_meta.direction = Direction::from(custom_properties.get("direction"));
        tile_meta.custom_properties = custom_properties;
        tile_meta.collision_shapes = collision_shapes;

        tile_metas.push(tile_meta);
    }

    tile_metas
}
