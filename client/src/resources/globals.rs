use crate::args::Args;
use crate::battle::BattleMeta;
use crate::lua_api::BattleLuaApi;
use crate::packages::*;
use crate::render::ui::{GlyphAtlas, PackageListing};
use crate::render::{
    BackgroundPipeline, MapPipeline, PostProcessAdjust, PostProcessAdjustConfig,
    PostProcessColorBlindness, PostProcessGhosting, SpritePipelineCollection,
};
use crate::resources::*;
use crate::saves::{BattleRecording, Config, GlobalSave};
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use packets::structures::FileHash;
use std::collections::HashSet;
use std::sync::Arc;

use super::restrictions::Restrictions;

pub struct Globals {
    pub config: Config,
    pub snap_resize: bool,
    pub post_process_adjust_config: PostProcessAdjustConfig,
    pub post_process_ghosting: f32,
    pub post_process_color_blindness: u8,
    pub global_save: GlobalSave,
    pub restrictions: Restrictions,
    pub card_recipes: CardRecipes,
    pub player_packages: PackageManager<PlayerPackage>,
    pub card_packages: PackageManager<CardPackage>,
    pub encounter_packages: PackageManager<EncounterPackage>,
    pub character_packages: PackageManager<CharacterPackage>,
    pub augment_packages: PackageManager<AugmentPackage>,
    pub status_packages: PackageManager<StatusPackage>,
    pub tile_state_packages: PackageManager<TileStatePackage>,
    pub library_packages: PackageManager<LibraryPackage>,
    pub resource_packages: PackageManager<ResourcePackage>,
    pub battle_api: BattleLuaApi,

    // recording
    pub battle_recording: Option<(BattleMeta, BattleRecording)>,

    // sounds
    pub audio: AudioManager,
    pub music: GlobalMusic,
    pub sfx: Box<GlobalSfx>,

    // assets
    pub assets: LocalAssetManager,
    pub glyph_atlas: Arc<GlyphAtlas>,

    // shaders
    pub sprite_pipeline_collection: SpritePipelineCollection,
    pub background_pipeline: BackgroundPipeline,
    pub background_sampler: Arc<TextureSampler>,
    pub map_pipeline: MapPipeline,

    // input emulation
    pub emulated_input: EmulatedInput,

    // networking
    pub network: Network,
    pub connected_to_server: bool,

    // debug
    pub debug_visible: bool,
}

impl Globals {
    pub fn new(game_io: &mut GameIO, args: Args) -> Self {
        let assets = LocalAssetManager::new(game_io);

        // load save
        let mut global_save = GlobalSave::load(&assets);

        // apply resource overrides
        let mut resource_packages: PackageManager<ResourcePackage> =
            PackageManager::new(PackageCategory::Resource);

        let resources_mod_path =
            ResourcePaths::data_folder_absolute(resource_packages.category().mod_path());

        resource_packages.load_packages_in_folder(
            &assets,
            PackageNamespace::Local,
            &resources_mod_path,
            |_, _| {},
        );
        resource_packages.apply(game_io, &mut global_save, &assets);

        // load config
        let config = Config::load(&assets);
        let music_volume = config.music_volume();
        let sfx_volume = config.sfx_volume();

        let audio = AudioManager::new(&config.audio_device)
            .with_music_volume(music_volume)
            .with_sfx_volume(sfx_volume);

        let window = game_io.window_mut();
        window.set_fullscreen(config.fullscreen);
        window.set_integer_scaling(config.integer_scaling);

        if config.lock_aspect_ratio {
            window.lock_resolution(TRUE_RESOLUTION);
        }

        let snap_resize = config.snap_resize;
        let post_process_adjust_config = PostProcessAdjustConfig::from_config(&config);
        let post_process_ghosting = config.ghosting as f32 * 0.01;
        let post_process_color_blindness = config.color_blindness;

        let enable_adjustment = post_process_adjust_config.should_enable();
        let enable_ghosting = config.ghosting > 0;
        let enable_color_blindness =
            config.color_blindness < PostProcessColorBlindness::TOTAL_OPTIONS;

        game_io.set_post_process_enabled::<PostProcessAdjust>(enable_adjustment);
        game_io.set_post_process_enabled::<PostProcessGhosting>(enable_ghosting);
        game_io.set_post_process_enabled::<PostProcessColorBlindness>(enable_color_blindness);

        Self {
            config,
            snap_resize,
            post_process_adjust_config,
            post_process_ghosting,
            post_process_color_blindness,
            global_save,
            restrictions: Restrictions::default(),
            card_recipes: CardRecipes::default(),
            player_packages: PackageManager::new(PackageCategory::Player),
            card_packages: PackageManager::new(PackageCategory::Card),
            encounter_packages: PackageManager::new(PackageCategory::Encounter),
            character_packages: PackageManager::new(PackageCategory::Character),
            augment_packages: PackageManager::new(PackageCategory::Augment),
            status_packages: PackageManager::new(PackageCategory::Status),
            tile_state_packages: PackageManager::new(PackageCategory::TileState),
            library_packages: PackageManager::new(PackageCategory::Library),
            resource_packages,
            battle_api: BattleLuaApi::new(),

            // recording
            battle_recording: None,

            // sounds
            audio,
            music: GlobalMusic::default(),
            sfx: Box::default(),

            // assets
            glyph_atlas: Arc::new(GlyphAtlas::new_default(game_io, &assets)),
            assets,

            // shaders
            sprite_pipeline_collection: SpritePipelineCollection::new(game_io),
            background_pipeline: BackgroundPipeline::new(game_io),
            background_sampler: TextureSampler::new(
                game_io,
                SamplingFilter::Nearest,
                EdgeSampling::Repeat,
            ),
            map_pipeline: MapPipeline::new(game_io),

            // input emulation
            emulated_input: EmulatedInput::default(),

            // networking
            network: Network::new(&args),
            connected_to_server: false,

            // debug
            debug_visible: false,
        }
    }

    pub fn packages(&self, namespace: PackageNamespace) -> impl Iterator<Item = &PackageInfo> {
        (self
            .augment_packages
            .packages(namespace)
            .map(|package| &package.package_info))
        .chain(
            self.card_packages
                .packages(namespace)
                .map(|package| &package.package_info),
        )
        .chain(
            self.encounter_packages
                .packages(namespace)
                .map(|package| &package.package_info),
        )
        .chain(
            self.library_packages
                .packages(namespace)
                .map(|package| &package.package_info),
        )
        .chain(
            self.player_packages
                .packages(namespace)
                .map(|package| &package.package_info),
        )
        .chain(
            self.character_packages
                .packages(namespace)
                .map(|package| &package.package_info),
        )
        .chain(
            self.status_packages
                .packages(namespace)
                .map(|package| &package.package_info),
        )
        .chain(
            self.tile_state_packages
                .packages(namespace)
                .map(|package| &package.package_info),
        )
    }

    pub fn load_virtual_package(
        &mut self,
        category: PackageCategory,
        namespace: PackageNamespace,
        hash: FileHash,
    ) -> Option<&PackageInfo> {
        let package_info = match category {
            PackageCategory::Augment => {
                self.augment_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::Card => {
                let mut package_info =
                    self.card_packages
                        .load_virtual_package(&self.assets, namespace, hash);

                // load recipes
                if let Some(info) = package_info {
                    let id = info.id.clone();
                    let package = self.card_packages.package(namespace, &id).unwrap();
                    self.card_recipes.load_from_package(namespace, package);

                    package_info = self
                        .card_packages
                        .package(namespace, &id)
                        .map(|p| p.package_info());
                }

                package_info
            }
            PackageCategory::Encounter => {
                self.encounter_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::Library => {
                self.library_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::Player => {
                self.player_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::Character => {
                log::error!("Attempt to load Character package");
                None
            }
            PackageCategory::Resource => {
                log::error!("Attempt to load virtual Resource package");
                None
            }
            PackageCategory::Status => {
                self.status_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::TileState => {
                self.tile_state_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
        }?;

        // load child packages
        // character packages are the only child packages for now
        let child_packages: Vec<_> = package_info.child_packages().collect();
        let id = package_info.id.clone();

        self.character_packages
            .load_child_packages(namespace, child_packages);

        self.package_info(category, namespace, &id)
    }

    pub fn load_package(
        &mut self,
        category: PackageCategory,
        namespace: PackageNamespace,
        path: &str,
    ) -> Option<&PackageInfo> {
        let package_info = match category {
            PackageCategory::Augment => {
                self.augment_packages
                    .load_package(&self.assets, namespace, path)
            }
            PackageCategory::Card => {
                let mut package_info =
                    self.card_packages
                        .load_package(&self.assets, namespace, path);

                // load recipes
                if let Some(info) = package_info {
                    let id = info.id.clone();
                    let package = self.card_packages.package(namespace, &id).unwrap();
                    self.card_recipes.load_from_package(namespace, package);

                    package_info = self
                        .card_packages
                        .package(namespace, &id)
                        .map(|p| p.package_info());
                }

                package_info
            }
            PackageCategory::Encounter => {
                self.encounter_packages
                    .load_package(&self.assets, namespace, path)
            }
            PackageCategory::Library => {
                self.library_packages
                    .load_package(&self.assets, namespace, path)
            }
            PackageCategory::Player => {
                self.player_packages
                    .load_package(&self.assets, namespace, path)
            }
            PackageCategory::Character => {
                log::error!("Attempt to load Character package");
                None
            }
            PackageCategory::Resource => {
                self.resource_packages
                    .load_package(&self.assets, namespace, path)
            }
            PackageCategory::Status => {
                self.status_packages
                    .load_package(&self.assets, namespace, path)
            }
            PackageCategory::TileState => {
                self.tile_state_packages
                    .load_package(&self.assets, namespace, path)
            }
        }?;

        // load child packages
        // character packages are the only child packages for now
        let child_packages: Vec<_> = package_info.child_packages().collect();
        let id = package_info.id.clone();

        self.character_packages
            .load_child_packages(namespace, child_packages);

        self.package_info(category, namespace, &id)
    }

    pub fn unload_package(
        &mut self,
        category: PackageCategory,
        namespace: PackageNamespace,
        id: &PackageId,
    ) {
        let Some(package_info) = self.package_info(category, namespace, id) else {
            return;
        };

        // get child packages before unloading
        // character packages are the only child packages for now
        let child_packages: Vec<_> = package_info.child_packages().collect();

        match category {
            PackageCategory::Augment => {
                self.augment_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::Card => {
                self.card_recipes.remove_associated(namespace, id);

                self.card_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::Encounter => {
                self.encounter_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::Library => {
                self.library_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::Player => {
                self.player_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::Character => {
                self.character_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::Resource => {
                self.resource_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::Status => {
                self.status_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::TileState => {
                self.tile_state_packages
                    .unload_package(&self.assets, namespace, id);
            }
        }

        // unload child packages
        for child_package in child_packages {
            let id = &child_package.id;

            self.character_packages
                .unload_package(&self.assets, namespace, id);
        }
    }

    // returns package info, and the namespace the package should be loaded with
    pub fn package_dependency_iter<I>(&self, iter: I) -> Vec<(&PackageInfo, PackageNamespace)>
    where
        I: IntoIterator<Item = (PackageCategory, PackageNamespace, PackageId)>,
    {
        let mut resolved = HashSet::new();

        let mut is_unresolved = move |triplet: &(PackageCategory, PackageNamespace, PackageId)| {
            resolved.insert(triplet.clone())
        };

        let resolve =
            |(package_category, ns, id): (PackageCategory, PackageNamespace, PackageId)| {
                Some((
                    self.package_or_fallback_info(package_category, ns, &id)?,
                    ns,
                ))
            };

        let mut package_infos = Vec::new();
        let mut prev_package_infos: Vec<_> = iter
            .into_iter()
            .filter(&mut is_unresolved)
            .flat_map(resolve)
            .collect();

        // loop until requirements are satisfied
        while !prev_package_infos.is_empty() {
            let latest_package_infos: Vec<_> = prev_package_infos
                .iter()
                .flat_map(|(package_info, ns)| {
                    package_info
                        .requirements
                        .iter()
                        .map(move |(category, id)| (*category, *ns, id.clone()))
                })
                .filter(&mut is_unresolved)
                .flat_map(resolve)
                .collect();

            package_infos.extend(prev_package_infos);
            prev_package_infos = latest_package_infos;
        }

        package_infos
    }

    pub fn package_info(
        &self,
        category: PackageCategory,
        namespace: PackageNamespace,
        id: &PackageId,
    ) -> Option<&PackageInfo> {
        match category {
            PackageCategory::Encounter => self
                .encounter_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Augment => self
                .augment_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Card => self
                .card_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Character => self
                .character_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Library => self
                .library_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Player => self
                .player_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Resource => self
                .resource_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Status => self
                .status_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::TileState => self
                .tile_state_packages
                .package(namespace, id)
                .map(|package| package.package_info()),
        }
    }

    pub fn package_or_fallback_info(
        &self,
        category: PackageCategory,
        namespace: PackageNamespace,
        id: &PackageId,
    ) -> Option<&PackageInfo> {
        match category {
            PackageCategory::Encounter => self
                .encounter_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Augment => self
                .augment_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Card => self
                .card_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Character => self
                .character_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Library => self
                .library_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Player => self
                .player_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Resource => self
                .resource_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::Status => self
                .status_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
            PackageCategory::TileState => self
                .tile_state_packages
                .package_or_fallback(namespace, id)
                .map(|package| package.package_info()),
        }
    }

    pub fn create_package_listing(
        &self,
        category: PackageCategory,
        id: &PackageId,
    ) -> Option<PackageListing> {
        let namespace = PackageNamespace::Local;

        match category {
            PackageCategory::Encounter => self
                .encounter_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
            PackageCategory::Augment => self
                .augment_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
            PackageCategory::Card => self
                .card_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
            PackageCategory::Character => self
                .character_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
            PackageCategory::Library => self
                .library_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
            PackageCategory::Player => self
                .player_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
            PackageCategory::Resource => self
                .resource_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
            PackageCategory::Status => self
                .status_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
            PackageCategory::TileState => self
                .tile_state_packages
                .package(namespace, id)
                .map(|package| package.create_package_listing()),
        }
    }

    pub fn namespaces(&self) -> impl Iterator<Item = PackageNamespace> + '_ {
        let mut namespace_set = HashSet::new();

        self.library_packages
            .namespaces()
            .chain(self.augment_packages.namespaces())
            .chain(self.encounter_packages.namespaces())
            .chain(self.card_packages.namespaces())
            .chain(self.character_packages.namespaces())
            .chain(self.player_packages.namespaces())
            .chain(self.library_packages.namespaces())
            .chain(self.status_packages.namespaces())
            .chain(self.tile_state_packages.namespaces())
            .filter(move |ns| namespace_set.insert(*ns))
    }

    pub fn remove_namespace(&mut self, namespace: PackageNamespace) {
        self.card_recipes.remove_namespace(namespace);

        self.augment_packages
            .remove_namespace(&self.assets, namespace);

        self.encounter_packages
            .remove_namespace(&self.assets, namespace);

        self.card_packages.remove_namespace(&self.assets, namespace);

        self.character_packages
            .remove_namespace(&self.assets, namespace);

        self.library_packages
            .remove_namespace(&self.assets, namespace);

        self.player_packages
            .remove_namespace(&self.assets, namespace);

        self.resource_packages
            .remove_namespace(&self.assets, namespace);

        self.status_packages
            .remove_namespace(&self.assets, namespace);

        self.tile_state_packages
            .remove_namespace(&self.assets, namespace);
    }

    pub fn resolve_package_download_path(
        &self,
        category: PackageCategory,
        id: &PackageId,
    ) -> String {
        if let Some(package) = self.package_info(category, PackageNamespace::Local, id) {
            package.base_path.clone()
        } else {
            let encoded_id = uri_encode(id.as_str());
            format!(
                "{}{}{}/",
                ResourcePaths::data_folder(),
                category.mod_path(),
                encoded_id
            )
        }
    }

    pub fn request_latest_hashes(
        &self,
    ) -> impl futures::Future<Output = Vec<(PackageCategory, PackageId, FileHash)>> {
        let ns = PackageNamespace::Local;
        let package_ids: Vec<_> = (self.augment_packages.package_ids(ns))
            .chain(self.encounter_packages.package_ids(ns))
            .chain(self.card_packages.package_ids(ns))
            .chain(self.library_packages.package_ids(ns))
            .chain(self.player_packages.package_ids(ns))
            .chain(self.resource_packages.package_ids(ns))
            .chain(self.status_packages.package_ids(ns))
            .chain(self.tile_state_packages.package_ids(ns))
            .map(|id| uri_encode(id.as_str()))
            .collect();

        let repo = self.config.package_repo.clone();

        async move {
            let mut hash_list = Vec::new();

            for chunk in package_ids.chunks(20) {
                let uri = format!("{repo}/api/mods/hashes?id={}", chunk.join("&id="));

                let Some(json) = crate::http::request_json(&uri).await else {
                    continue;
                };

                let Some(arr) = json.as_array() else {
                    continue;
                };

                let transform_iter = arr
                    .iter()
                    .flat_map(|value| {
                        Some((
                            value.get("category")?.as_str()?,
                            value.get("id")?.as_str()?,
                            value.get("hash")?.as_str()?,
                        ))
                    })
                    .flat_map(|(category, id, hash)| {
                        Some((
                            PackageCategory::from(category),
                            PackageId::from(id),
                            FileHash::from_hex(hash)?,
                        ))
                    });

                hash_list.extend(transform_iter);
            }

            hash_list
        }
    }
}
