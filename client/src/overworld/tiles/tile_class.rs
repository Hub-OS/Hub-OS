#[derive(Debug, PartialEq, Eq)]
pub enum TileClass {
    Conveyor,
    Stairs,
    Ice,
    Treadmill,
    Invisible,
    Arrow,
    Undefined,
}

impl From<&str> for TileClass {
    fn from(type_str: &str) -> TileClass {
        match type_str.to_lowercase().as_str() {
            "conveyor" => TileClass::Conveyor,
            "stairs" => TileClass::Stairs,
            "ice" => TileClass::Ice,
            "treadmill" => TileClass::Treadmill,
            "invisible" => TileClass::Invisible,
            "arrow" => TileClass::Arrow,
            _ => TileClass::Undefined,
        }
    }
}
