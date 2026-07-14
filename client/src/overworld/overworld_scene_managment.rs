use crate::overworld::AutoEmote;

#[derive(PartialEq, Eq, Clone, Copy)]
pub enum NextOverworldSceneCategory {
    Menu,
    Battle,
    Netplay,
    Transfer,
}

impl NextOverworldSceneCategory {
    pub fn auto_emote(&self) -> AutoEmote {
        match self {
            Self::Menu => AutoEmote::Menu,
            Self::Battle | Self::Netplay => AutoEmote::Battle,
            _ => AutoEmote::None,
        }
    }
}
