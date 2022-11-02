#[derive(Clone, Copy)]
pub enum ObjectType {
    CustomWarp,
    ServerWarp,
    PositionWarp,
    CustomServerWarp,
    HomeWarp,
    Board,
    Shop,
    Undefined,
}

impl ObjectType {
    pub fn is_solid(self) -> bool {
        !self.is_warp()
    }

    pub fn is_warp(self) -> bool {
        matches!(
            self,
            ObjectType::CustomWarp
                | ObjectType::ServerWarp
                | ObjectType::PositionWarp
                | ObjectType::CustomServerWarp
                | ObjectType::HomeWarp
        )
    }

    pub fn is_custom_warp(self) -> bool {
        matches!(self, ObjectType::CustomWarp | ObjectType::CustomServerWarp)
    }
}

impl From<&str> for ObjectType {
    fn from(type_str: &str) -> ObjectType {
        match type_str.to_lowercase().as_str() {
            "custom warp" => ObjectType::CustomWarp,
            "server warp" => ObjectType::ServerWarp,
            "position warp" => ObjectType::PositionWarp,
            "custom server warp" => ObjectType::CustomServerWarp,
            "home warp" => ObjectType::HomeWarp,
            "board" => ObjectType::Board,
            "shop" => ObjectType::Shop,
            _ => ObjectType::Undefined,
        }
    }
}
