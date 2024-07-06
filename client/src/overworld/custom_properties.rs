use std::collections::HashMap;

pub struct CustomProperties {
    properties: HashMap<String, String>,
}

impl CustomProperties {
    pub fn new() -> Self {
        Self {
            properties: HashMap::new(),
        }
    }

    pub fn has_property(&self, name: &str) -> bool {
        self.properties.contains_key(name)
    }

    pub fn get(&self, name: &str) -> &str {
        self.properties
            .get(name)
            .map(|value| value.as_str())
            .unwrap_or_default()
    }

    pub fn get_i32(&self, name: &str) -> i32 {
        let str_value = self.get(name);

        str_value.parse::<i32>().unwrap_or_default()
    }

    pub fn get_f32(&self, name: &str) -> f32 {
        let str_value = self.get(name);

        str_value.parse::<f32>().unwrap_or_default()
    }
}

impl From<roxmltree::Node<'_, '_>> for CustomProperties {
    fn from(properties_element: roxmltree::Node) -> CustomProperties {
        let mut properties = CustomProperties::new();

        for attribute in properties_element.attributes() {
            properties
                .properties
                .insert(attribute.name().to_owned(), attribute.value().to_owned());
        }

        for child in properties_element.children() {
            if child.tag_name().name() != "property" {
                continue;
            }

            let name = child.attribute("name").unwrap_or_default().to_lowercase();

            properties.properties.insert(
                name,
                child.attribute("value").unwrap_or_default().to_owned(),
            );
        }

        properties
    }
}
