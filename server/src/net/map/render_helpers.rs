use std::collections::HashMap;

fn escape(s: &str) -> String {
    use htmlentity::entity::{encode, CharacterSet, EncodeType, ICodedDataTrait};

    encode(
        s.as_bytes(),
        &EncodeType::NamedOrHex,
        &CharacterSet::SpecialChars,
    )
    .to_string()
    .unwrap_or_default()
}

pub fn render_custom_properties(custom_properties: &HashMap<String, String>) -> String {
    if custom_properties.is_empty() {
        return String::default();
    }

    let mut property_strings = Vec::<String>::with_capacity(custom_properties.len());

    for (name, value) in custom_properties {
        let escaped_name = escape(name);
        let escaped_value = escape(value);

        let property_string = format!("<property name={escaped_name:?} value={escaped_value:?}/>");
        property_strings.push(property_string);
    }

    format!("<properties>{}</properties>", property_strings.join(""))
}
