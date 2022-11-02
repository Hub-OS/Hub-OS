use crate::parse_util::parse_or_default;

use super::*;

pub trait Shape: Send + Sync {
    fn x(&self) -> f32;
    fn y(&self) -> f32;
    fn width(&self) -> f32;
    fn height(&self) -> f32;
    fn rotation(&self) -> f32;

    // point is within the shape or on an edge
    fn intersects(&self, point: (f32, f32)) -> bool;

    fn rotate_around(&self, point: &mut (f32, f32)) {
        if self.rotation() != 0.0 {
            let relative_x = point.0 - self.x();
            let relative_y = point.1 - self.y();

            let distance = relative_x.hypot(relative_y).sqrt();
            let relative_radians = relative_y.atan2(relative_x);
            let rotation_radians = (self.rotation() / 180.0).to_radians();
            let radians = relative_radians + rotation_radians;

            // set x + y to a position rotated around self.x() + self.y()
            point.0 = radians.cos() * distance + self.x();
            point.1 = radians.sin() * distance + self.y();
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

        for (i, c) in points_str.chars().enumerate() {
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
