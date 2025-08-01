// used to create object entities, not too useful of a file after creation

use super::ObjectData;
use crate::overworld::Map;
use structures::parse_util::parse_or_default;
use structures::shapes::{Ellipse, Point, Polygon, Rect, Shape};

pub struct ShapeObject {
    pub data: ObjectData,
    pub shape: Box<dyn Shape>,
}

impl ShapeObject {
    pub fn new(id: u32, shape: Box<dyn Shape>) -> ShapeObject {
        ShapeObject {
            data: ObjectData::new(id),
            shape,
        }
    }

    pub fn intersects(&self, _world: &Map, point: (f32, f32)) -> bool {
        self.shape.intersects(point)
    }
}

impl From<roxmltree::Node<'_, '_>> for ShapeObject {
    fn from(element: roxmltree::Node) -> ShapeObject {
        ShapeObject {
            data: ObjectData::from(element),
            shape: shape_from_xml(element).unwrap_or_else(|| Box::new(Point::new(0.0, 0.0))),
        }
    }
}

pub fn shape_from_xml(object_element: roxmltree::Node) -> Option<Box<dyn Shape>> {
    if object_element.tag_name().name() != "object" {
        return None;
    }

    let x = parse_or_default::<f32>(object_element.attribute("x"));
    let y = parse_or_default::<f32>(object_element.attribute("y"));
    let width = parse_or_default::<f32>(object_element.attribute("width"));
    let height = parse_or_default::<f32>(object_element.attribute("height"));
    let rotation = parse_or_default::<f32>(object_element.attribute("rotation"));

    let child_element = object_element
        .children()
        .find(|child| !child.tag_name().name().is_empty());

    let child_name = child_element.map(|child| child.tag_name().name());

    if child_name == Some("polygon") {
        let polygon_element = child_element.unwrap();
        let points_str = polygon_element.attribute("points").unwrap_or_default();

        let mut polygon = Box::new(Polygon::new(x, y, rotation));

        let mut slice_start = 0;
        let mut x = 0.0;

        for (i, c) in points_str.char_indices() {
            match c {
                ',' => {
                    x = points_str[slice_start..i]
                        .parse::<f32>()
                        .unwrap_or_default();

                    slice_start = i + 1;
                }
                ' ' => {
                    let y = points_str[slice_start..i]
                        .parse::<f32>()
                        .unwrap_or_default();

                    polygon.add_point((x, y));
                    slice_start = i + 1;
                }
                _ => {}
            }
        }

        // add the last point
        let y = points_str[slice_start..].parse::<f32>().unwrap_or_default();

        polygon.add_point((x, y));

        Some(polygon)
    } else if width == 0.0 && height == 0.0 {
        Some(Box::new(Point::new(x, y)))
    } else if child_name == Some("ellipse") {
        Some(Box::new(Ellipse::new(x, y, width, height, rotation)))
    } else {
        Some(Box::new(Rect::new(x, y, width, height, rotation)))
    }
}
