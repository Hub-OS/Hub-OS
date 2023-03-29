use std::borrow::Cow;

#[derive(Debug, Clone)]
pub enum PacketScope<'a> {
    All,
    Area(Cow<'a, str>),
    Client(Cow<'a, str>),
}

impl<'a> PacketScope<'a> {
    pub fn into_owned(self) -> PacketScope<'static> {
        match self {
            PacketScope::All => PacketScope::All,
            PacketScope::Area(id) => PacketScope::Area(Cow::Owned(id.into_owned())),
            PacketScope::Client(id) => PacketScope::Client(Cow::Owned(id.into_owned())),
        }
    }

    pub fn is_visible_to(&self, area_id: &str, player_id: &str) -> bool {
        match self {
            PacketScope::All => true,
            PacketScope::Area(id) => id == area_id,
            PacketScope::Client(id) => id == player_id,
        }
    }
}
