// used to create object entities, not too useful of a file after creation

use super::ObjectData;
use crate::overworld::shapes::{shape_from_xml, Point, Shape};
use crate::overworld::Map;

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
