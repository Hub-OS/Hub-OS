use super::{ResourcePaths, SoundBuffer};
use field_count::FieldCount;
use rustysynth::SoundFont;
use std::io::Cursor;
use std::sync::Arc;

#[derive(Default, FieldCount)]
pub struct GlobalMusic {
    pub sound_font: Option<Arc<SoundFont>>,
    pub main_menu: Vec<SoundBuffer>,
    pub customize: Vec<SoundBuffer>,
    pub battle: Vec<SoundBuffer>,
    pub overworld: Vec<SoundBuffer>,
}

impl GlobalMusic {
    pub fn load_with<E>(
        sound_font_bytes: Vec<u8>,
        mut load: impl FnMut(&str) -> Result<Vec<SoundBuffer>, E>,
    ) -> Result<Self, E> {
        Ok(Self {
            sound_font: Self::load_sound_font(sound_font_bytes),
            main_menu: load(ResourcePaths::MAIN_MENU_MUSIC)?,
            customize: load(ResourcePaths::CUSTOMIZE_MUSIC)?,
            battle: load(ResourcePaths::BATTLE_MUSIC)?,
            overworld: load(ResourcePaths::OVERWORLD_MUSIC)?,
        })
    }

    fn load_sound_font(sound_font_bytes: Vec<u8>) -> Option<Arc<SoundFont>> {
        let mut cursor = Cursor::new(sound_font_bytes);

        match SoundFont::new(&mut cursor) {
            Ok(sound_font) => Some(Arc::new(sound_font)),
            Err(err) => {
                log::error!("{err}");
                None
            }
        }
    }

    pub fn total() -> usize {
        Self::field_count()
    }
}
