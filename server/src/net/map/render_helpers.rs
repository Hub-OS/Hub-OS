use std::collections::HashMap;

pub fn render_custom_properties(custom_properties: &HashMap<String, String>) -> String {
    if custom_properties.is_empty() {
        return String::default();
    }

    let mut property_strings = Vec::<String>::with_capacity(custom_properties.len());

    for (name, value) in custom_properties {
        let property_string = format!("<property name={:?} value={:?}/>", name, value);
        property_strings.push(property_string);
    }

    format!("<properties>{}</properties>", property_strings.join(""))
}
