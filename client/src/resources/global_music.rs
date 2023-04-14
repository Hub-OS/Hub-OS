use super::{ResourcePaths, SoundBuffer};
use field_count::FieldCount;

#[derive(Default, FieldCount)]
pub struct GlobalMusic {
    pub main_menu: SoundBuffer,
    pub customize: SoundBuffer,
    pub battle: SoundBuffer,
}

impl GlobalMusic {
    pub fn load_with(mut load: impl FnMut(&str) -> SoundBuffer) -> Self {
        Self {
            main_menu: load(ResourcePaths::MAIN_MENU_MUSIC),
            customize: load(ResourcePaths::CUSTOMIZE_MUSIC),
            battle: load(ResourcePaths::BATTLE_MUSIC),
        }
    }

    pub fn total() -> usize {
        Self::field_count()
    }
}
