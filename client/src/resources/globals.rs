use crate::args::Args;
use crate::battle::BattleProps;
use crate::lua_api::BattleLuaApi;
use crate::packages::*;
use crate::render::ui::PackageListing;
use crate::render::{
    Animator, BackgroundPipeline, PostProcessAdjust, PostProcessAdjustConfig,
    PostProcessColorBlindness, PostProcessGhosting, SpritePipelineCollection,
};
use crate::resources::*;
use crate::saves::{BlockGrid, Config, GlobalSave};
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use packets::structures::FileHash;
use std::collections::HashSet;
use std::sync::Arc;

pub struct Globals {
    pub config: Config,
    pub post_process_adjust_config: PostProcessAdjustConfig,
    pub post_process_ghosting: f32,
    pub post_process_color_blindness: u8,
    pub global_save: GlobalSave,
    pub player_packages: PackageManager<PlayerPackage>,
    pub card_packages: PackageManager<CardPackage>,
    pub battle_packages: PackageManager<BattlePackage>,
    pub character_packages: PackageManager<CharacterPackage>,
    pub augment_packages: PackageManager<AugmentPackage>,
    pub library_packages: PackageManager<LibraryPackage>,
    pub battle_api: BattleLuaApi,

    // sounds
    pub audio: AudioManager,
    pub main_menu_music: SoundBuffer,
    pub customize_music: SoundBuffer,
    pub battle_music: SoundBuffer,
    pub start_game_sfx: SoundBuffer,
    pub cursor_move_sfx: SoundBuffer,
    pub cursor_select_sfx: SoundBuffer,
    pub cursor_cancel_sfx: SoundBuffer,
    pub cursor_error_sfx: SoundBuffer,
    pub menu_close_sfx: SoundBuffer,
    pub page_turn_sfx: SoundBuffer,
    pub text_blip_sfx: SoundBuffer,
    pub customize_start_sfx: SoundBuffer,
    pub customize_empty_sfx: SoundBuffer,
    pub customize_block_sfx: SoundBuffer,
    pub customize_complete_sfx: SoundBuffer,
    pub transmission_sfx: SoundBuffer,
    pub warp_sfx: SoundBuffer,
    pub battle_transition_sfx: SoundBuffer,
    pub appear_sfx: SoundBuffer,
    pub card_select_open_sfx: SoundBuffer,
    pub card_select_confirm_sfx: SoundBuffer,
    pub form_select_open_sfx: SoundBuffer,
    pub form_select_close_sfx: SoundBuffer,
    pub turn_gauge_sfx: SoundBuffer,
    pub time_freeze_sfx: SoundBuffer,
    pub tile_break_sfx: SoundBuffer,
    pub trap_sfx: SoundBuffer,
    pub shine_sfx: SoundBuffer,
    pub transform_select_sfx: SoundBuffer,
    pub transform_sfx: SoundBuffer,
    pub transform_revert_sfx: SoundBuffer,
    pub attack_charging_sfx: SoundBuffer,
    pub attack_charged_sfx: SoundBuffer,
    pub player_deleted_sfx: SoundBuffer,
    pub hurt_sfx: SoundBuffer,
    pub explode_sfx: SoundBuffer,

    // assets
    pub assets: LocalAssetManager,
    pub font_texture: Arc<Texture>,
    pub font_animator: Arc<Animator>,

    // shaders
    pub sprite_pipeline_collection: SpritePipelineCollection,
    pub background_pipeline: BackgroundPipeline,
    pub background_sampler: Arc<TextureSampler>,

    // networking
    pub network: Network,
    pub connected_to_server: bool,

    // debug
    pub debug_visible: bool,
}

impl Globals {
    pub fn new(game_io: &mut GameIO, args: Args) -> Self {
        let assets = LocalAssetManager::new(game_io);
        let font_texture = assets.texture(game_io, ResourcePaths::FONTS);

        let config = Config::load(&assets);
        let music_volume = config.music_volume();
        let sfx_volume = config.sfx_volume();

        if config.fullscreen {
            game_io.window_mut().set_fullscreen(true);
        }

        if config.lock_aspect_ratio {
            game_io.window_mut().lock_resolution(TRUE_RESOLUTION);
        }

        let post_process_adjust_config = PostProcessAdjustConfig::from_config(&config);
        let post_process_ghosting = config.ghosting as f32 * 0.01;
        let post_process_color_blindness = config.color_blindness;

        let graphics = game_io.graphics_mut();

        let enable_adjustment = post_process_adjust_config.should_enable();
        let enable_ghosting = config.ghosting > 0;
        let enable_color_blindness =
            config.color_blindness < PostProcessColorBlindness::TOTAL_OPTIONS;

        graphics.set_post_process_enabled::<PostProcessAdjust>(enable_adjustment);
        graphics.set_post_process_enabled::<PostProcessGhosting>(enable_ghosting);
        graphics.set_post_process_enabled::<PostProcessColorBlindness>(enable_color_blindness);

        Self {
            config,
            post_process_adjust_config,
            post_process_ghosting,
            post_process_color_blindness,
            global_save: GlobalSave::load(&assets),
            player_packages: PackageManager::new(PackageCategory::Player),
            card_packages: PackageManager::new(PackageCategory::Card),
            battle_packages: PackageManager::new(PackageCategory::Battle),
            character_packages: PackageManager::new(PackageCategory::Character),
            augment_packages: PackageManager::new(PackageCategory::Augment),
            library_packages: PackageManager::new(PackageCategory::Library),
            battle_api: BattleLuaApi::new(),

            // sounds
            audio: AudioManager::new()
                .with_music_volume(music_volume)
                .with_sfx_volume(sfx_volume),
            main_menu_music: assets.audio(ResourcePaths::MAIN_MENU_MUSIC),
            customize_music: assets.audio(ResourcePaths::CUSTOMIZE_MUSIC),
            battle_music: assets.audio(ResourcePaths::BATTLE_MUSIC),
            start_game_sfx: assets.audio(ResourcePaths::START_GAME_SFX),
            cursor_move_sfx: assets.audio(ResourcePaths::CURSOR_MOVE_SFX),
            cursor_select_sfx: assets.audio(ResourcePaths::CURSOR_SELECT_SFX),
            cursor_cancel_sfx: assets.audio(ResourcePaths::CURSOR_CANCEL_SFX),
            cursor_error_sfx: assets.audio(ResourcePaths::CURSOR_ERROR_SFX),
            menu_close_sfx: assets.audio(ResourcePaths::MENU_CLOSE_SFX),
            page_turn_sfx: assets.audio(ResourcePaths::PAGE_TURN_SFX),
            text_blip_sfx: assets.audio(ResourcePaths::TEXT_BLIP_SFX),
            customize_start_sfx: assets.audio(ResourcePaths::CUSTOMIZE_START_SFX),
            customize_empty_sfx: assets.audio(ResourcePaths::CUSTOMIZE_EMPTY_SFX),
            customize_block_sfx: assets.audio(ResourcePaths::CUSTOMIZE_BLOCK_SFX),
            customize_complete_sfx: assets.audio(ResourcePaths::CUSTOMIZE_COMPLETE_SFX),
            transmission_sfx: assets.audio(ResourcePaths::TRANSMISSION_SFX),
            warp_sfx: assets.audio(ResourcePaths::WARP_SFX),
            battle_transition_sfx: assets.audio(ResourcePaths::BATTLE_TRANSITION_SFX),
            appear_sfx: assets.audio(ResourcePaths::APPEAR_SFX),
            card_select_open_sfx: assets.audio(ResourcePaths::CARD_SELECT_OPEN_SFX),
            card_select_confirm_sfx: assets.audio(ResourcePaths::CARD_SELECT_CONFIRM_SFX),
            form_select_open_sfx: assets.audio(ResourcePaths::FORM_SELECT_OPEN_SFX),
            form_select_close_sfx: assets.audio(ResourcePaths::FORM_SELECT_CLOSE_SFX),
            turn_gauge_sfx: assets.audio(ResourcePaths::TURN_GAUGE_SFX),
            time_freeze_sfx: assets.audio(ResourcePaths::TIME_FREEZE_SFX),
            tile_break_sfx: assets.audio(ResourcePaths::TILE_BREAK_SFX),
            trap_sfx: assets.audio(ResourcePaths::TRAP_SFX),
            shine_sfx: assets.audio(ResourcePaths::SHINE_SFX),
            transform_select_sfx: assets.audio(ResourcePaths::TRANSFORM_SELECT_SFX),
            transform_sfx: assets.audio(ResourcePaths::TRANSFORM_SFX),
            transform_revert_sfx: assets.audio(ResourcePaths::TRANSFORM_REVERT_SFX),
            attack_charging_sfx: assets.audio(ResourcePaths::ATTACK_CHARGING_SFX),
            attack_charged_sfx: assets.audio(ResourcePaths::ATTACK_CHARGED_SFX),
            player_deleted_sfx: assets.audio(ResourcePaths::PLAYER_DELETED_SFX),
            hurt_sfx: assets.audio(ResourcePaths::HURT_SFX),
            explode_sfx: assets.audio(ResourcePaths::EXPLODE_SFX),

            // assets
            font_texture,
            font_animator: Arc::new(Animator::load_new(&assets, ResourcePaths::FONTS_ANIMATION)),
            assets,

            // shaders
            sprite_pipeline_collection: SpritePipelineCollection::new(game_io),
            background_pipeline: BackgroundPipeline::new(game_io),
            background_sampler: TextureSampler::new(
                game_io,
                SamplingFilter::Nearest,
                EdgeSampling::Repeat,
            ),

            // networking
            network: Network::new(&args),
            connected_to_server: false,

            // debug
            debug_visible: false,
        }
    }

    // updated by the Overlay
    pub fn tick(&mut self) {
        self.network.tick();
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
                self.card_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::Battle => {
                self.battle_packages
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
        }?;

        // load child packages
        // character packages are the only child packages for now
        let child_packages: Vec<_> = package_info.child_packages().collect();
        let id = package_info.id.clone();

        self.character_packages
            .load_child_packages(namespace, child_packages);

        self.package_or_fallback_info(category, namespace, &id)
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
            PackageCategory::Card => self
                .card_packages
                .load_package(&self.assets, namespace, path),
            PackageCategory::Battle => {
                self.battle_packages
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
        }?;

        // load child packages
        // character packages are the only child packages for now
        let child_packages: Vec<_> = package_info.child_packages().collect();
        let id = package_info.id.clone();

        self.character_packages
            .load_child_packages(namespace, child_packages);

        self.package_or_fallback_info(category, namespace, &id)
    }

    pub fn unload_package(
        &mut self,
        category: PackageCategory,
        namespace: PackageNamespace,
        id: &PackageId,
    ) {
        let Some(package_info) = self.package_or_fallback_info(category, namespace, id) else {
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
                self.card_packages
                    .unload_package(&self.assets, namespace, id);
            }
            PackageCategory::Battle => {
                self.battle_packages
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
        }

        // unload child packages
        for child_package in child_packages {
            let id = &child_package.id;

            self.character_packages
                .unload_package(&self.assets, namespace, id);
        }
    }

    pub fn battle_dependencies(&self, game_io: &GameIO, props: &BattleProps) -> Vec<&PackageInfo> {
        let player_package_iter = props
            .player_setups
            .iter()
            .map(|setup| setup.player_package.package_info.triplet());

        let card_package_iter = props.player_setups.iter().flat_map(|setup| {
            let ns = setup.namespace();

            let card_iter = setup.deck.cards.iter();
            card_iter.map(move |card| (PackageCategory::Card, ns, card.package_id.clone()))
        });

        let augment_package_iter = props.player_setups.iter().flat_map(|setup| {
            let ns = setup.namespace();

            BlockGrid::new(ns)
                .with_blocks(game_io, setup.blocks.clone())
                .augments(game_io)
                .map(move |(augment, _)| {
                    (
                        PackageCategory::Augment,
                        ns,
                        augment.package_info.id.clone(),
                    )
                })
                .collect::<Vec<_>>()
        });

        let battle_package_iter = std::iter::once(props.battle_package)
            .flatten()
            .map(|package| package.package_info().triplet());

        let package_iter = player_package_iter
            .chain(card_package_iter)
            .chain(augment_package_iter)
            .chain(battle_package_iter);

        self.package_dependency_iter(package_iter)
    }

    pub fn package_dependency_iter<I>(&self, iter: I) -> Vec<&PackageInfo>
    where
        I: IntoIterator<Item = (PackageCategory, PackageNamespace, PackageId)>,
    {
        let mut resolved = HashSet::new();

        let mut is_unresolved = move |triplet: &(PackageCategory, PackageNamespace, PackageId)| {
            resolved.insert(triplet.clone())
        };

        let resolve =
            |(package_category, ns, id): (PackageCategory, PackageNamespace, PackageId)| {
                self.package_or_fallback_info(package_category, ns, &id)
            };

        let mut package_infos = Vec::new();
        let mut prev_package_infos: Vec<&PackageInfo> = iter
            .into_iter()
            .filter(&mut is_unresolved)
            .flat_map(resolve)
            .collect();

        // loop until requirements are satisfied
        while !prev_package_infos.is_empty() {
            let latest_package_infos: Vec<_> = prev_package_infos
                .iter()
                .flat_map(|package_info| {
                    let ns = package_info.namespace;

                    package_info
                        .requirements
                        .iter()
                        .map(move |(package_category, id)| (*package_category, ns, id.clone()))
                })
                .filter(&mut is_unresolved)
                .flat_map(resolve)
                .collect();

            package_infos.extend(prev_package_infos);
            prev_package_infos = latest_package_infos;
        }

        package_infos
    }

    pub fn package_or_fallback_info(
        &self,
        category: PackageCategory,
        namespace: PackageNamespace,
        id: &PackageId,
    ) -> Option<&PackageInfo> {
        match category {
            PackageCategory::Battle => self
                .battle_packages
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
        }
    }

    pub fn create_package_listing(
        &self,
        category: PackageCategory,
        id: &PackageId,
    ) -> Option<PackageListing> {
        let namespace = PackageNamespace::Local;

        match category {
            PackageCategory::Battle => self
                .battle_packages
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
        }
    }

    pub fn remote_namespaces(&self) -> Vec<PackageNamespace> {
        let mut namespace_set = HashSet::new();

        self.library_packages
            .namespaces()
            .chain(self.battle_packages.namespaces())
            .chain(self.augment_packages.namespaces())
            .chain(self.card_packages.namespaces())
            .chain(self.player_packages.namespaces())
            .chain(self.character_packages.namespaces())
            .chain(self.library_packages.namespaces())
            .filter(|ns| ns.is_remote())
            .for_each(|namespace| {
                namespace_set.insert(namespace);
            });

        Vec::from_iter(namespace_set)
    }

    pub fn remove_namespace(&mut self, namespace: PackageNamespace) {
        self.battle_packages
            .remove_namespace(&self.assets, namespace);

        self.augment_packages
            .remove_namespace(&self.assets, namespace);

        self.card_packages.remove_namespace(&self.assets, namespace);

        self.character_packages
            .remove_namespace(&self.assets, namespace);

        self.library_packages
            .remove_namespace(&self.assets, namespace);

        self.player_packages
            .remove_namespace(&self.assets, namespace);
    }

    pub fn resolve_package_download_path(
        &self,
        category: PackageCategory,
        id: &PackageId,
    ) -> String {
        if let Some(package) = self.package_or_fallback_info(category, PackageNamespace::Local, id)
        {
            package.base_path.clone()
        } else {
            let encoded_id = uri_encode(id.as_str());
            format!("{}{}/", category.path(), encoded_id)
        }
    }

    pub fn request_latest_hashes(
        &self,
    ) -> impl futures::Future<Output = Vec<(PackageCategory, PackageId, FileHash)>> {
        let ns = PackageNamespace::Local;
        let package_ids: Vec<_> = (self.battle_packages.package_ids(ns))
            .chain(self.augment_packages.package_ids(ns))
            .chain(self.card_packages.package_ids(ns))
            .chain(self.character_packages.package_ids(ns))
            .chain(self.library_packages.package_ids(ns))
            .chain(self.player_packages.package_ids(ns))
            .map(|id| uri_encode(id.as_str()))
            .collect();

        let repo = &self.config.package_repo;
        let uri = format!("{repo}/api/mods/hashes?id={}", package_ids.join("&id="));

        async move {
            let Some(json) = crate::http::request_json(&uri).await else {
                return Vec::new();
            };

            let Some(arr) = json.as_array() else {
                return Vec::new();
            };

            arr.iter()
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
                })
                .collect()
        }
    }
}
