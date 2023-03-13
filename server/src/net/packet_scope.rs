#[derive(Clone, Copy)]
pub enum PacketScope<'a> {
    All,
    Area(&'a str),
    Client(&'a str),
}

#[derive(Clone)]
pub enum PacketScopeOwned {
    All,
    Area(String),
    Client(String),
}

impl<'a> PacketScope<'a> {
    pub fn to_owned(self) -> PacketScopeOwned {
        match self {
            PacketScope::All => PacketScopeOwned::All,
            PacketScope::Area(id) => PacketScopeOwned::Area(id.to_string()),
            PacketScope::Client(id) => PacketScopeOwned::Client(id.to_string()),
        }
    }

    pub fn is_visible_to(self, area_id: &str, player_id: &str) -> bool {
        match self {
            PacketScope::All => true,
            PacketScope::Area(id) => id == area_id,
            PacketScope::Client(id) => id == player_id,
        }
    }
}
