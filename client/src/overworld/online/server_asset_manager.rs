use crate::render::ui::GlyphAtlas;
use crate::resources::*;
use base64::Engine;
use framework::prelude::*;
use packets::address_parsing::{uri_decode, uri_encode};
use packets::structures::{AssetDataType, FileHash, TextureAnimPathPair};
use std::cell::RefCell;
use std::collections::HashMap;
use std::fs;
use std::sync::Arc;

struct ServerAssetDownload {
    remote_path: String,
    hash: FileHash,
    expected_size: usize,
    save_to_disk: bool,
    data_type: AssetDataType,
    data: Vec<u8>,
}

struct CachedServerAsset {
    remote_path: String,
    local_path: String,
    hash: FileHash,
    data: Option<Vec<u8>>,
}

pub struct StoredServerAsset {
    pub remote_path: String,
    pub hash: FileHash,
}

const BASE64_ENGINE: base64::engine::GeneralPurpose = base64::engine::GeneralPurpose::new(
    &base64::alphabet::URL_SAFE,
    base64::engine::general_purpose::NO_PAD,
);

impl CachedServerAsset {
    fn new(path_prefix: &str, remote_path: String, hash: FileHash) -> Self {
        let encoded_remote_path = uri_encode(&remote_path);

        let mut local_path = path_prefix.to_string();
        local_path += &BASE64_ENGINE.encode(hash.as_bytes());
        local_path += " ";
        local_path += &encoded_remote_path;

        Self {
            remote_path,
            local_path,
            hash,
            data: None,
        }
    }

    fn decode_local(path_prefix: &str, local_name: &str) -> Option<Self> {
        let local_path = format!("{path_prefix}{local_name}");

        let (hash_str, encoded_remote_path) = local_name.split_once(' ')?;
        let hash_bytes = BASE64_ENGINE.decode(hash_str).ok()?;

        Some(Self {
            local_path,
            remote_path: uri_decode(encoded_remote_path)?,
            hash: FileHash::from_prehashed(hash_bytes.try_into().ok()?),
            data: None,
        })
    }
}

pub struct ServerAssetManager {
    path_prefix: String,
    stored_assets: RefCell<HashMap<String, CachedServerAsset>>,
    textures: RefCell<HashMap<String, Arc<Texture>>>,
    sounds: RefCell<HashMap<String, SoundBuffer>>,
    glyph_atlases: RefCell<HashMap<TextureAnimPathPair<'static>, Arc<GlyphAtlas>>>,
    current_download: Option<ServerAssetDownload>,
}

impl ServerAssetManager {
    pub fn new(game_io: &GameIO, address: &str) -> Self {
        let path_prefix = Self::resolve_prefix(address);

        // find stored assets
        let assets = Self::find_stored_assets(&path_prefix);

        // setup texture map
        let mut textures = HashMap::new();

        let local_assets = &Globals::from_resources(game_io).assets;
        textures.insert(
            ResourcePaths::BLANK.to_string(),
            local_assets.texture(game_io, ResourcePaths::BLANK),
        );

        // setup sound map
        let mut sounds = HashMap::new();

        sounds.insert(
            ResourcePaths::BLANK.to_string(),
            local_assets.audio(game_io, ResourcePaths::BLANK),
        );

        Self {
            path_prefix,
            stored_assets: RefCell::new(assets),
            textures: RefCell::new(textures),
            sounds: RefCell::new(sounds),
            glyph_atlases: Default::default(),
            current_download: None,
        }
    }

    fn resolve_prefix(address: &str) -> String {
        let mut prefix = packets::address_parsing::strip_data(address).replace(':', "_p");
        prefix = uri_encode(&prefix);

        ResourcePaths::clean_folder(&(ResourcePaths::server_cache_folder() + prefix.as_str()))
    }

    fn find_stored_assets(path: &str) -> HashMap<String, CachedServerAsset> {
        let mut assets = HashMap::new();

        if let Err(err) = fs::create_dir_all(path) {
            log::error!("Failed to create cache folder in \"{path}\": {err}");
            return assets;
        }

        let entries = match fs::read_dir(path) {
            Ok(entries) => entries,
            Err(err) => {
                log::error!("Failed to find cache folder \"{path}\": {err}");
                return assets;
            }
        };

        for entry in entries {
            let entry = match entry {
                Ok(entry) => entry,
                Err(err) => {
                    log::error!("Error while reading from cache folder in \"{path}\": {err}");
                    return assets;
                }
            };

            if let Ok(metadata) = entry.metadata()
                && metadata.is_dir()
            {
                continue;
            }

            let file_name = entry.file_name();
            let Some(file_name_str) = file_name.to_str() else {
                let file_path = entry.path();
                log::warn!("Removing invalid cache file: {file_path:?}");
                let _ = std::fs::remove_file(file_path);
                continue;
            };

            if let Some(asset) = CachedServerAsset::decode_local(path, file_name_str) {
                assets.insert(asset.remote_path.clone(), asset);
            } else {
                let file_path = entry.path();
                log::warn!("Removing invalid cache file: {file_path:?}");
                let _ = std::fs::remove_file(file_path);
            }
        }

        assets
    }

    pub fn delete_asset(&self, remote_path: &str) {
        let Some(asset) = self.stored_assets.borrow_mut().remove(remote_path) else {
            return;
        };

        let _ = fs::remove_file(asset.local_path);
    }

    pub fn store_asset(&self, remote_path: String, hash: FileHash, data: Vec<u8>, write: bool) {
        let mut asset = CachedServerAsset::new(&self.path_prefix, remote_path.clone(), hash);

        if write {
            let _ = fs::write(&asset.local_path, &data);
        }

        asset.data = Some(data);

        self.stored_assets.borrow_mut().insert(remote_path, asset);
    }

    pub fn start_download(
        &mut self,
        remote_path: String,
        hash: FileHash,
        expected_size: usize,
        data_type: AssetDataType,
        write: bool,
    ) {
        self.current_download = Some(ServerAssetDownload {
            remote_path,
            hash,
            expected_size,
            save_to_disk: write,
            data_type,
            data: Vec::new(),
        });
    }

    pub fn receive_download_data(&mut self, game_io: &GameIO, data: Vec<u8>) {
        let Some(download) = &mut self.current_download else {
            log::warn!("Received data for a server asset when no download has started");
            return;
        };

        download.data.extend(data);

        if download.data.len() < download.expected_size {
            // still working on this file
            return;
        }

        let download = self.current_download.take().unwrap();

        if download.data.len() > download.expected_size {
            log::warn!(
                "Downloaded size for {:?} is larger than expected, discarding file",
                download.remote_path
            );

            return;
        }

        let mut data = download.data;

        if download.data_type == AssetDataType::CompressedText {
            use flate2::read::ZlibDecoder;
            use std::io::Read;

            let source = std::mem::take(&mut data);

            let mut decoder = ZlibDecoder::new(&*source);

            if let Err(e) = decoder.read_to_end(&mut data) {
                log::error!("Failed to decompress text from server: {e}");
            }
        }

        let remote_path = download.remote_path;

        self.store_asset(
            remote_path.clone(),
            download.hash,
            data,
            download.save_to_disk,
        );

        match download.data_type {
            AssetDataType::Texture => {
                // cache as texture
                self.textures.borrow_mut().remove(&remote_path);
                self.texture(game_io, &remote_path);
            }
            AssetDataType::Audio => {
                // cache as audio
                self.sounds.borrow_mut().remove(&remote_path);
                self.audio(game_io, &remote_path);
            }
            _ => {}
        }
    }

    pub fn stored_assets(&self) -> Vec<StoredServerAsset> {
        self.stored_assets
            .borrow()
            .values()
            .map(|asset| StoredServerAsset {
                remote_path: asset.remote_path.clone(),
                hash: asset.hash,
            })
            .collect()
    }

    pub fn file_hash(&self, path: &str) -> Option<FileHash> {
        let stored_assets = self.stored_assets.borrow();
        Some(stored_assets.get(path)?.hash)
    }
}

impl AssetManager for ServerAssetManager {
    fn local_path(&self, path: &str) -> String {
        self.stored_assets
            .borrow()
            .get(path)
            .map(|asset| asset.local_path.clone())
            .unwrap_or_default()
    }

    fn binary(&self, path: &str) -> Vec<u8> {
        if path == ResourcePaths::BLANK {
            return Vec::new();
        }

        let mut stored_assets = self.stored_assets.borrow_mut();
        let Some(asset) = stored_assets.get_mut(path) else {
            return Vec::new();
        };

        match &asset.data {
            Some(data) => data.clone(),
            None => {
                let res = fs::read(&asset.local_path);

                if let Err(err) = &res {
                    log::warn!("Failed to load {path:?}: {err}");
                }

                let bytes = res.unwrap_or_default();

                asset.data = Some(bytes.clone());

                bytes
            }
        }
    }

    fn binary_silent(&self, path: &str) -> Vec<u8> {
        if path == ResourcePaths::BLANK {
            return Vec::new();
        }

        let mut stored_assets = self.stored_assets.borrow_mut();
        let Some(asset) = stored_assets.get_mut(path) else {
            return Vec::new();
        };

        match &asset.data {
            Some(data) => data.clone(),
            None => {
                let res = fs::read(&asset.local_path);
                let bytes = res.unwrap_or_default();
                asset.data = Some(bytes.clone());
                bytes
            }
        }
    }

    fn text(&self, path: &str) -> String {
        if path == ResourcePaths::BLANK {
            return String::new();
        }

        let bytes = self.binary(path);
        let res = String::from_utf8(bytes);

        if let Err(err) = &res {
            log::warn!("Failed to read {path:?} as a string: {err}");
        }

        res.unwrap_or_default()
    }

    fn text_silent(&self, path: &str) -> String {
        if path == ResourcePaths::BLANK {
            return String::new();
        }

        let bytes = self.binary(path);
        let res = String::from_utf8(bytes);
        res.unwrap_or_default()
    }

    fn texture(&self, game_io: &GameIO, path: &str) -> Arc<Texture> {
        let mut textures = self.textures.borrow_mut();

        if let Some(texture) = textures.get(path) {
            texture.clone()
        } else {
            let bytes = self.binary(path);
            let texture = match Texture::load_from_memory(game_io, &bytes) {
                Ok(texture) => texture,
                Err(_) => textures.get(ResourcePaths::BLANK).unwrap().clone(),
            };

            textures.insert(path.to_string(), texture.clone());
            texture
        }
    }

    fn audio(&self, game_io: &GameIO, path: &str) -> SoundBuffer {
        let mut sounds = self.sounds.borrow_mut();

        if let Some(sound) = sounds.get(path) {
            sound.clone()
        } else {
            let sound = SoundBuffer::decode(game_io, self.binary(path));
            sounds.insert(path.to_string(), sound.clone());
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
