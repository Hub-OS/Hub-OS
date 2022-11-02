pub enum TileShadow {
    Automatic,
    Always,
    Never,
}

impl From<&str> for TileShadow {
    fn from(type_str: &str) -> TileShadow {
        match type_str.to_lowercase().as_str() {
            "always" => TileShadow::Always,
            "never" => TileShadow::Never,

            _ => TileShadow::Automatic,
        }
    }
}
