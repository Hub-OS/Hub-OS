use super::*;
use framework::prelude::*;
use packets::structures::{AssetDataType, FileHash};
use std::cell::RefCell;
use std::collections::HashMap;
use std::fs;
use std::sync::Arc;

#[derive(Clone)]
pub struct VirtualZipMeta {
    pub virtual_prefix: String,
}

struct VirtualZipTracking {
    meta: VirtualZipMeta,
    virtual_files: Vec<String>,
    use_count: isize,
    bytes: Vec<u8>,
}

pub struct LocalAssetManager {
    loaded_zips: RefCell<HashMap<FileHash, VirtualZipTracking>>,
    text_cache: RefCell<HashMap<String, String>>,
    texture_cache: RefCell<HashMap<String, Arc<Texture>>>,
    sound_cache: RefCell<HashMap<String, SoundBuffer>>,
}

impl LocalAssetManager {
    pub fn new(game_io: &GameIO) -> Self {
        let text = HashMap::from([(ResourcePaths::BLANK.to_string(), String::new())]);

        let textures = HashMap::from([(
            ResourcePaths::BLANK.to_string(),
            RenderTarget::new(game_io, UVec2::new(1, 1))
                .texture()
                .clone(),
        )]);

        let sounds = HashMap::from([(ResourcePaths::BLANK.to_string(), SoundBuffer::new_empty())]);

        Self {
            loaded_zips: RefCell::new(HashMap::new()),
            text_cache: RefCell::new(text),
            texture_cache: RefCell::new(textures),
            sound_cache: RefCell::new(sounds),
        }
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
            log::error!("virtual zip usage below zero: {}", hash);
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
                log::debug!("deleting virtual zip: {hash}");

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

        crate::zip::extract(&bytes, |path, mut file| {
            let virtual_path = meta.virtual_prefix.clone() + path.as_str();

            let data_type = AssetDataType::from_path_str(&virtual_path);

            let res = match data_type {
                AssetDataType::Text => {
                    let mut text = String::new();
                    let read_result = file.read_to_string(&mut text);

                    text_cache.insert(virtual_path.clone(), text);
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
                            log::error!("failed to load {:?} in {}: {}", file.name(), hash, err);
                        }
                    }

                    read_result
                }
                AssetDataType::Audio => {
                    let mut bytes = Vec::new();
                    let read_result = file.read_to_end(&mut bytes);

                    let sound = SoundBuffer(Arc::new(bytes));
                    sound_cache.insert(virtual_path.clone(), sound);
                    virtual_files.push(virtual_path);

                    read_result
                }
                AssetDataType::Data => Ok(0),
                AssetDataType::Unknown => Ok(0),
            };

            if let Err(err) = res {
                log::error!("failed to load {:?} in {}: {}", file.name(), hash, err);
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
            log::warn!("failed to load {:?}: {}", path, err);
        }

        res.unwrap_or_default()
    }

    fn text(&self, path: &str) -> String {
        let mut text_cache = self.text_cache.borrow_mut();

        if let Some(text) = text_cache.get(path) {
            text.clone()
        } else {
            let res = fs::read_to_string(path);

            if let Err(err) = &res {
                log::warn!("failed to load {:?}: {}", path, err);
            }

            let text = res.unwrap_or_default();

            text_cache.insert(path.to_string(), text.clone());
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
                    log::warn!("failed to load {:?}: {}", path, err);
                    texture_cache.get(ResourcePaths::BLANK).unwrap().clone()
                }
            };

            texture_cache.insert(path.to_string(), texture.clone());
            texture
        }
    }

    fn audio(&self, path: &str) -> SoundBuffer {
        let mut sound_cache = self.sound_cache.borrow_mut();

        if let Some(sound) = sound_cache.get(path) {
            sound.clone()
        } else {
            let bytes = fs::read(path).unwrap_or_default();
            let sound = SoundBuffer(Arc::new(bytes));
            sound_cache.insert(path.to_string(), sound.clone());
            sound
        }
    }
}
