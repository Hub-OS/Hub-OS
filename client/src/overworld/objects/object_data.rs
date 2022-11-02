use super::ObjectType;
use crate::overworld::CustomProperties;
use crate::parse_util::parse_or_default;
use framework::prelude::Vec2;

pub struct ObjectData {
    pub id: u32,
    pub name: String,
    pub class: String,
    pub object_type: ObjectType,
    pub visible: bool,
    pub solid: bool,
    pub position: Vec2,
    pub size: Vec2,
    pub rotation: f32,
    pub custom_properties: CustomProperties,
}

impl ObjectData {
    pub fn new(id: u32) -> ObjectData {
        ObjectData {
            id,
            name: String::new(),
            class: String::new(),
            object_type: ObjectType::Undefined,
            visible: true,
            solid: true,
            position: Vec2::ZERO,
            size: Vec2::ZERO,
            rotation: 0.0,
            custom_properties: CustomProperties::new(),
        }
    }
}

impl From<roxmltree::Node<'_, '_>> for ObjectData {
    fn from(element: roxmltree::Node) -> ObjectData {
        let class = element
            .attribute("class")
            .or_else(|| element.attribute("type"))
            .unwrap_or_default();

        ObjectData {
            id: parse_or_default::<u32>(element.attribute("id")),
            name: element.attribute("name").unwrap_or_default().to_string(),
            class: class.to_string(),
            object_type: ObjectType::from(class),
            position: Vec2::new(
                parse_or_default::<f32>(element.attribute("x")),
                parse_or_default::<f32>(element.attribute("y")),
            ),
            size: Vec2::new(
                parse_or_default::<f32>(element.attribute("width")),
                parse_or_default::<f32>(element.attribute("height")),
            ),
            rotation: parse_or_default::<f32>(element.attribute("rotation")),
            visible: element.attribute("visible") != Some("0"),
            solid: true,

            custom_properties: element
                .children()
                .find(|child| child.tag_name().name() == "properties")
                .map(CustomProperties::from)
                .unwrap_or_else(CustomProperties::new),
        }
    }
}
