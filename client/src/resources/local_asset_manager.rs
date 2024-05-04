use crate::render::ui::GlyphAtlas;

use super::*;
use framework::prelude::*;
use packets::structures::{AssetDataType, FileHash, TextureAnimPathPair};
use std::cell::RefCell;
use std::collections::HashMap;
use std::fs;
use std::sync::Arc;

#[derive(Clone)]
pub struct VirtualZipMeta {
    pub virtual_prefix: String,
}

#[derive(Clone)]
struct VirtualZipTracking {
    meta: VirtualZipMeta,
    virtual_files: Vec<Arc<str>>,
    use_count: isize,
    bytes: Vec<u8>,
}

#[derive(Clone)]
pub struct LocalAssetManager {
    loaded_zips: RefCell<HashMap<FileHash, VirtualZipTracking>>,
    text_cache: RefCell<HashMap<Arc<str>, Arc<str>>>,
    texture_cache: RefCell<HashMap<Arc<str>, Arc<Texture>>>,
    sound_cache: RefCell<HashMap<Arc<str>, SoundBuffer>>,
    glyph_atlases: RefCell<HashMap<TextureAnimPathPair<'static>, Arc<GlyphAtlas>>>,
}

impl LocalAssetManager {
    pub fn new(game_io: &GameIO) -> Self {
        let text: HashMap<Arc<str>, Arc<str>> =
            HashMap::from([(Arc::from(ResourcePaths::BLANK), Arc::from(""))]);

        let textures: HashMap<Arc<str>, Arc<Texture>> = HashMap::from([(
            ResourcePaths::BLANK.into(),
            RenderTarget::new(game_io, UVec2::new(1, 1))
                .texture()
                .clone(),
        )]);

        let sounds: HashMap<Arc<str>, SoundBuffer> =
            HashMap::from([(ResourcePaths::BLANK.into(), SoundBuffer::new_empty())]);

        Self {
            loaded_zips: RefCell::new(HashMap::new()),
            text_cache: RefCell::new(text),
            texture_cache: RefCell::new(textures),
            sound_cache: RefCell::new(sounds),
            glyph_atlases: Default::default(),
        }
    }

    pub fn contains_virtual_zip(&self, hash: &FileHash) -> bool {
        self.loaded_zips.borrow().contains_key(hash)
    }

    pub fn virtual_zip_meta(&self, hash: &FileHash) -> Option<VirtualZipMeta> {
        self.loaded_zips
            .borrow()
            .get(hash)
            .map(|tracking| tracking.meta.clone())
    }

    pub fn virtual_zip_bytes(&self, hash: &FileHash) -> Option<Vec<u8>> {
        self.loaded_zips
            .borrow()
            .get(hash)
            .map(|tracking| tracking.bytes.clone())
    }

    pub fn add_virtual_zip_use(&self, hash: &FileHash) {
        if let Some(tracking) = self.loaded_zips.borrow_mut().get_mut(hash) {
            tracking.use_count += 1;
        }
    }

    pub fn remove_virtual_zip_use(&self, hash: &FileHash) {
        let mut loaded_zips = self.loaded_zips.borrow_mut();

        if let Some(tracking) = loaded_zips.get_mut(hash) {
            tracking.use_count -= 1;
        } else {
            log::error!("Virtual zip usage below zero: {}", hash);
        }
    }

    pub fn remove_unused_virtual_zips(&self) {
        let mut loaded_zips = self.loaded_zips.borrow_mut();

        let mut pending_removal = Vec::new();

        for (hash, tracking) in loaded_zips.iter() {
            if tracking.use_count == 0 {
                pending_removal.push(*hash);
            }
        }

        for hash in pending_removal {
            loaded_zips.remove(&hash);

            if let Some(tracking) = loaded_zips.remove(&hash) {
                log::debug!("Deleting virtual zip: {hash}");

                let mut text_cache = self.text_cache.borrow_mut();
                let mut texture_cache = self.texture_cache.borrow_mut();
                let mut sound_cache = self.sound_cache.borrow_mut();

                for file in tracking.virtual_files {
                    text_cache.remove(&file);
                    texture_cache.remove(&file);
                    sound_cache.remove(&file);
                }
            }
        }
    }

    /// Returns path prefix for the virtual zip
    pub fn load_virtual_zip(
        &self,
        game_io: &GameIO,
        hash: FileHash,
        bytes: Vec<u8>,
    ) -> VirtualZipMeta {
        let mut loaded_zips = self.loaded_zips.borrow_mut();

        if let Some(tracking) = loaded_zips.get(&hash) {
            log::debug!("{hash} is already loaded. skipping...");

            return tracking.meta.clone();
        }

        loaded_zips.remove(&hash);

        let mut text_cache = self.text_cache.borrow_mut();
        let mut texture_cache = self.texture_cache.borrow_mut();
        let mut sound_cache = self.sound_cache.borrow_mut();

        use std::io::Read;

        let virtual_prefix =
            ResourcePaths::clean_folder(&format!("{}{}", ResourcePaths::VIRTUAL_PREFIX, hash));

        let meta = VirtualZipMeta { virtual_prefix };

        let mut virtual_files = Vec::new();

        packets::zip::extract(&bytes, |path, mut file| {
            let virtual_path = meta.virtual_prefix.clone() + path.as_str();
            let virtual_path = Arc::<str>::from(virtual_path);

            let data_type = AssetDataType::from_path_str(&virtual_path);

            let res = match data_type {
                AssetDataType::Text => {
                    let mut text = String::new();
                    let read_result = file.read_to_string(&mut text);

                    text_cache.insert(virtual_path.clone(), text.into());
                    virtual_files.push(virtual_path);

                    read_result
                }
                AssetDataType::CompressedText => unreachable!(),
                AssetDataType::Texture => {
                    let mut bytes = Vec::new();
                    let read_result = file.read_to_end(&mut bytes);

                    match Texture::load_from_memory(game_io, &bytes) {
                        Ok(texture) => {
                            texture_cache.insert(virtual_path.clone(), texture);
                            virtual_files.push(virtual_path);
                        }
                        Err(err) => {
                            log::error!("Failed to load {:?} in {}: {}", file.name(), hash, err);
                        }
                    }

                    read_result
                }
                AssetDataType::Audio => {
                    let mut bytes = Vec::new();
                    let read_result = file.read_to_end(&mut bytes);

                    let sound = SoundBuffer::decode(game_io, bytes);
                    sound_cache.insert(virtual_path.clone(), sound);
                    virtual_files.push(virtual_path);

                    read_result
                }
                AssetDataType::Data => Ok(0),
                AssetDataType::Unknown => Ok(0),
            };

            if let Err(err) = res {
                log::error!("Failed to load {:?} in {}: {}", file.name(), hash, err);
            }
        });

        let tracking = VirtualZipTracking {
            meta: meta.clone(),
            virtual_files,
            use_count: 0,
            bytes,
        };

        loaded_zips.insert(hash, tracking);

        meta
    }

    pub fn non_midi_audio(&self, path: &str) -> SoundBuffer {
        let mut sound_cache = self.sound_cache.borrow_mut();

        if let Some(sound) = sound_cache.get(path) {
            sound.clone()
        } else {
            let bytes = fs::read(path).unwrap_or_default();
            let sound = SoundBuffer::decode_non_midi(bytes);
            sound_cache.insert(path.into(), sound.clone());
            sound
        }
    }

    pub fn clear_local_mod_assets(&self) {
        let mut text_cache = self.text_cache.borrow_mut();
        let mut texture_cache = self.texture_cache.borrow_mut();
        let mut sound_cache = self.sound_cache.borrow_mut();

        let base_mod_folder = ResourcePaths::game_folder().to_string() + "mods";
        let base_mod_folder = ResourcePaths::clean_folder(&base_mod_folder);

        text_cache.retain(|key, _| !key.starts_with(&base_mod_folder));
        texture_cache.retain(|key, _| !key.starts_with(&base_mod_folder));
        sound_cache.retain(|key, _| !key.starts_with(&base_mod_folder));
    }

    pub fn override_cache(&self, game_io: &GameIO, path: &str, file_path: &str) {
        let Ok(bytes) = std::fs::read(file_path) else {
            return;
        };

        match AssetDataType::from_path_str(path) {
            AssetDataType::Text => {
                let mut text_cache = self.text_cache.borrow_mut();

                let text = String::from_utf8_lossy(&bytes);
                let text = Arc::<str>::from(text);

                text_cache.insert(path.into(), text.clone());

                let absolute_path = ResourcePaths::game_folder().to_owned() + path;
                text_cache.insert(absolute_path.into(), text);
            }
            AssetDataType::CompressedText => {
                // only used for server sent data
                unreachable!()
            }
            AssetDataType::Texture => {
                let mut texture_cache = self.texture_cache.borrow_mut();

                if let Ok(texture) = Texture::load_from_memory(game_io, &bytes) {
                    texture_cache.insert(path.into(), texture.clone());

                    let absolute_path = ResourcePaths::game_folder().to_owned() + path;
                    texture_cache.insert(absolute_path.into(), texture);
                } else {
                    log::error!("Failed to load texture: {file_path}");
                }
            }
            AssetDataType::Audio => {
                let mut sound_cache = self.sound_cache.borrow_mut();

                let sound = SoundBuffer::decode(game_io, bytes);
                sound_cache.insert(path.into(), sound.clone());

                let absolute_path = ResourcePaths::game_folder().to_owned() + path;
                sound_cache.insert(absolute_path.into(), sound);
            }
            AssetDataType::Data => {
                // used for zips, ignore
            }
            AssetDataType::Unknown => {
                log::error!("Attempted to override resource with unknown file type: {file_path}");
            }
        }
    }
}

impl AssetManager for LocalAssetManager {
    fn local_path(&self, path: &str) -> String {
        path.to_string()
    }

    fn binary(&self, path: &str) -> Vec<u8> {
        if path == ResourcePaths::BLANK {
            return Vec::new();
        }

        let res = fs::read(path);

        if let Err(err) = &res {
            log::warn!("Failed to load {:?}: {}", ResourcePaths::shorten(path), err);
        }

        res.unwrap_or_default()
    }

    fn text(&self, path: &str) -> String {
        let mut text_cache = self.text_cache.borrow_mut();

        if let Some(text) = text_cache.get(path) {
            text.to_string()
        } else {
            let res = fs::read_to_string(path);

            if let Err(err) = &res {
                log::warn!("Failed to load {:?}: {}", ResourcePaths::shorten(path), err);
            }

            let text = res.unwrap_or_default();

            text_cache.insert(path.into(), text.clone().into());
            text
        }
    }

    fn texture(&self, game_io: &GameIO, path: &str) -> Arc<Texture> {
        let mut texture_cache = self.texture_cache.borrow_mut();

        if let Some(texture) = texture_cache.get(path) {
            texture.clone()
        } else {
            let bytes = fs::read(path).unwrap_or_default();
            let texture = match Texture::load_from_memory(game_io, &bytes) {
                Ok(texture) => texture,
                Err(err) => {
                    log::warn!("Failed to load {:?}: {}", ResourcePaths::shorten(path), err);
                    texture_cache.get(ResourcePaths::BLANK).unwrap().clone()
                }
            };

            texture_cache.insert(path.into(), texture.clone());
            texture
        }
    }

    fn audio(&self, game_io: &GameIO, path: &str) -> SoundBuffer {
        let mut sound_cache = self.sound_cache.borrow_mut();

        if let Some(sound) = sound_cache.get(path) {
            sound.clone()
        } else {
            let bytes = fs::read(path).unwrap_or_default();
            let sound = SoundBuffer::decode(game_io, bytes);
            sound_cache.insert(path.into(), sound.clone());
            sound
        }
    }

    fn glyph_atlas(
        &self,
        game_io: &GameIO,
        texture_path: &str,
        animation_path: &str,
    ) -> Arc<GlyphAtlas> {
        let mut atlases = self.glyph_atlases.borrow_mut();

        let key = TextureAnimPathPair {
            texture: texture_path.into(),
            animation: animation_path.into(),
        };

        if let Some(atlas) = atlases.get(&key) {
            atlas.clone()
        } else {
            // note: self.glyph_atlases is still borrowed,
            // should be fine as long as GlyphAtlas::new() doesn't try accessing it

            let glyph_atlas =
                Arc::new(GlyphAtlas::new(game_io, self, texture_path, animation_path));

            atlases.insert(key.own(), glyph_atlas.clone());
            glyph_atlas
        }
    }
}
