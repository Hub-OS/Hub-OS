use super::{ResourcePaths, SoundBuffer};
use field_count::FieldCount;
use rustysynth::SoundFont;
use std::io::Cursor;
use std::sync::Arc;

#[derive(Default)]
pub struct MusicTracks(Vec<(String, SoundBuffer)>);

impl MusicTracks {
    pub fn pick_random(&self) -> Option<&SoundBuffer> {
        use rand::seq::IndexedRandom;

        let (name, buffer) = self.0.choose(&mut rand::rng())?;

        log::info!("Selected {name}");

        Some(buffer)
    }

    pub fn contains(&self, buffer: &SoundBuffer) -> bool {
        self.0.iter().any(|(_, b)| b == buffer)
    }
}

#[derive(Default, FieldCount)]
pub struct GlobalMusic {
    pub sound_font: Option<Arc<SoundFont>>,
    pub main_menu: MusicTracks,
    pub customize: MusicTracks,
    pub battle: MusicTracks,
    pub overworld: MusicTracks,
    pub credits: MusicTracks,
}

impl GlobalMusic {
    pub fn load_with<E>(
        sound_font_bytes: Vec<u8>,
        mut load: impl FnMut(&str) -> Result<Vec<(String, SoundBuffer)>, E>,
    ) -> Result<Self, E> {
        Ok(Self {
            sound_font: Self::load_sound_font(sound_font_bytes),
            main_menu: MusicTracks(load(ResourcePaths::MAIN_MENU_MUSIC)?),
            customize: MusicTracks(load(ResourcePaths::CUSTOMIZE_MUSIC)?),
            battle: MusicTracks(load(ResourcePaths::BATTLE_MUSIC)?),
            overworld: MusicTracks(load(ResourcePaths::OVERWORLD_MUSIC)?),
            credits: MusicTracks(load(ResourcePaths::CREDITS_MUSIC)?),
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
