use super::Tile;
use crate::helpers::unwrap_and_parse_or_default;
use std::collections::HashMap;

pub struct MapObjectSpecification {
    pub name: String,
    pub class: String,
    pub visible: bool,
    pub x: f32,
    pub y: f32,
    pub layer: usize,
    pub width: f32,
    pub height: f32,
    pub rotation: f32,
    pub custom_properties: HashMap<String, String>,
    pub data: MapObjectData,
}

#[derive(Clone)]
pub struct MapObject {
    pub id: u32,
    pub name: String,
    pub class: String,
    pub visible: bool,
    pub x: f32,
    pub y: f32,
    pub layer: usize,
    pub width: f32,
    pub height: f32,
    pub rotation: f32,
    pub custom_properties: HashMap<String, String>,
    pub data: MapObjectData,
}

#[derive(Clone)]
pub enum MapObjectData {
    Point,
    Rect,
    Ellipse,
    Polyline { points: Vec<(f32, f32)> },
    Polygon { points: Vec<(f32, f32)> },
    TileObject { tile: Tile },
}

impl MapObject {
    pub fn from(element: &minidom::Element, layer: usize, scale_x: f32, scale_y: f32) -> MapObject {
        let name = element.attr("name").unwrap_or_default().to_string();
        let class = element
            .attr("class")
            .or_else(|| element.attr("type"))
            .unwrap_or_default()
            .to_string();
        let visible: bool = element.attr("visible").unwrap_or_default() != "0";
        let id: u32 = unwrap_and_parse_or_default(element.attr("id"));
        let gid: u32 = unwrap_and_parse_or_default(element.attr("gid"));
        let x = unwrap_and_parse_or_default::<f32>(element.attr("x")) * scale_x;
        let y = unwrap_and_parse_or_default::<f32>(element.attr("y")) * scale_y;
        let width = unwrap_and_parse_or_default::<f32>(element.attr("width")) * scale_x;
        let height = unwrap_and_parse_or_default::<f32>(element.attr("height")) * scale_y;
        let rotation: f32 = unwrap_and_parse_or_default(element.attr("rotation"));

        let mut custom_properties = HashMap::new();

        if let Some(properties_element) = element.get_child("properties", minidom::NSChoice::Any) {
            for child in properties_element.children() {
                if child.name() != "property" {
                    continue;
                }

                let name = child.attr("name").unwrap_or_default();
                let value = child
                    .attr("value")
                    .map(|value| value.to_string())
                    .unwrap_or_else(|| child.text());

                custom_properties.insert(name.to_string(), value);
            }
        }

        let data = if gid != 0 {
            MapObjectData::TileObject {
                tile: Tile::from(gid),
            }
        } else if element.has_child("polyline", minidom::NSChoice::Any) {
            let points_element = element
                .get_child("polyline", minidom::NSChoice::Any)
                .unwrap();

            let points_str = points_element.attr("points").unwrap_or_default();

            MapObjectData::Polyline {
                points: read_points(points_str),
            }
        } else if element.has_child("polygon", minidom::NSChoice::Any) {
            let points_element = element
                .get_child("polygon", minidom::NSChoice::Any)
                .unwrap();

            let points_str = points_element.attr("points").unwrap_or_default();

            MapObjectData::Polygon {
                points: read_points(points_str),
            }
        } else if element.has_child("ellipse", minidom::NSChoice::Any) {
            MapObjectData::Ellipse
        } else if width == 0.0 && height == 0.0 {
            MapObjectData::Point
        } else {
            MapObjectData::Rect
        };

        MapObject {
            id,
            name,
            class,
            visible,
            x,
            y,
            layer,
            width,
            height,
            rotation,
            custom_properties,
            data,
        }
    }

    pub fn render(&mut self, scale_x: f32, scale_y: f32) -> String {
        use super::render_helpers::render_custom_properties;

        let name_string = if !self.name.is_empty() {
            format!(" name={:?}", self.name)
        } else {
            String::default()
        };

        let class_string = if !self.class.is_empty() {
            // todo: update to class when the client properly supports it
            format!(" type={:?}", self.class)
        } else {
            String::default()
        };

        let visible_str = if !self.visible { " visible=\"0\"" } else { "" };

        let dimensions_string = if self.width != 0.0 && self.height != 0.0 {
            format!(
                " width=\"{}\" height=\"{}\"",
                self.width / scale_x,
                self.height / scale_y
            )
        } else {
            String::default()
        };

        let properties_string = render_custom_properties(&self.custom_properties);

        let mut data_string = String::new();
        let mut gid_string = String::new();

        match &self.data {
            MapObjectData::Point | MapObjectData::Rect => {}
            MapObjectData::Ellipse => {
                data_string = String::from("<ellipse/>");
            }
            MapObjectData::Polyline { points } => {
                data_string = format!("<polyline points={:?}/>", render_points(points));
            }
            MapObjectData::Polygon { points } => {
                data_string = format!("<polygon points={:?}/>", render_points(points));
            }
            MapObjectData::TileObject { tile } => {
                gid_string = format!(" gid=\"{}\"", tile.compress());
            }
        }

        let rotation_string = if self.rotation != 0.0 {
            format!(" rotation=\"{}\"", self.rotation)
        } else {
            String::default()
        };

        format!(
            "\
      <object id=\"{}\"{}{}{} x=\"{}\" y=\"{}\"{}{}{}>\
        {}\
        {}\
      </object>\
      ",
            self.id,
            name_string,
            class_string,
            gid_string,
            self.x / scale_x,
            self.y / scale_y,
            dimensions_string,
            rotation_string,
            visible_str,
            properties_string,
            data_string,
        )
    }
}

fn read_points(points_str: &str) -> Vec<(f32, f32)> {
    points_str
        .split(' ')
        .flat_map(|point_str| {
            let comma_index = point_str.find(',')?;

            Some((
                point_str[0..comma_index].parse::<f32>().unwrap_or(0.0),
                point_str[comma_index + 1..].parse::<f32>().unwrap_or(0.0),
            ))
        })
        .collect::<Vec<(f32, f32)>>()
}

fn render_points(points: &[(f32, f32)]) -> String {
    points
        .iter()
        .map(|point| format!("{},{}", point.0, point.1))
        .collect::<Vec<String>>()
        .join(" ")
}
