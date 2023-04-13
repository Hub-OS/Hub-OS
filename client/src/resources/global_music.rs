use super::{AssetManager, LocalAssetManager, ResourcePaths, SoundBuffer};

#[derive(Default)]
pub struct GlobalMusic {
    pub main_menu: SoundBuffer,
    pub customize: SoundBuffer,
    pub battle: SoundBuffer,
}

impl GlobalMusic {
    pub fn new(assets: &LocalAssetManager) -> Self {
        Self {
            main_menu: assets.audio(ResourcePaths::MAIN_MENU_MUSIC),
            customize: assets.audio(ResourcePaths::CUSTOMIZE_MUSIC),
            battle: assets.audio(ResourcePaths::BATTLE_MUSIC),
        }
    }
}
