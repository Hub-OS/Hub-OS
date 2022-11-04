use crate::args::Args;
use crate::battle::BattleProps;
use crate::lua_api::BattleLuaApi;
use crate::packages::*;
use crate::render::{Animator, BackgroundPipeline, SpritePipelineCollection};
use crate::resources::*;
use crate::saves::{Config, GlobalSave};
use framework::prelude::*;
use packets::structures::FileHash;
use std::sync::Arc;

pub struct Globals {
    pub config: Config,
    pub global_save: GlobalSave,
    pub player_packages: PackageManager<PlayerPackage>,
    pub card_packages: PackageManager<CardPackage>,
    pub battle_packages: PackageManager<BattlePackage>,
    pub block_packages: PackageManager<BlockPackage>,
    pub library_packages: PackageManager<LibraryPackage>,
    pub battle_api: BattleLuaApi,

    // sounds
    pub audio: AudioManager,
    pub battle_music: SoundBuffer,
    pub cursor_move_sfx: SoundBuffer,
    pub cursor_select_sfx: SoundBuffer,
    pub cursor_cancel_sfx: SoundBuffer,
    pub cursor_error_sfx: SoundBuffer,
    pub page_turn_sfx: SoundBuffer,
    pub text_blip_sfx: SoundBuffer,
    pub warp_sfx: SoundBuffer,
    pub appear_sfx: SoundBuffer,
    pub card_select_open_sfx: SoundBuffer,
    pub card_select_confirm_sfx: SoundBuffer,
    pub turn_guage_sfx: SoundBuffer,
    pub attack_charging_sfx: SoundBuffer,
    pub attack_charged_sfx: SoundBuffer,
    pub player_deleted_sfx: SoundBuffer,

    // assets
    pub assets: LocalAssetManager,
    pub font_texture: Arc<Texture>,
    pub font_animator: Arc<Animator>,

    // shaders
    pub sprite_pipeline: SpritePipeline<SpriteInstanceData>,
    pub sprite_pipeline_collection: SpritePipelineCollection,
    pub background_pipeline: BackgroundPipeline,
    pub default_sampler: Arc<TextureSampler>,
    pub background_sampler: Arc<TextureSampler>,

    // networking
    pub network: Network,
}

impl Globals {
    pub fn new(game_io: &mut GameIO<Globals>, args: Args) -> Self {
        let assets = LocalAssetManager::new(game_io);
        let font_texture = assets.texture(game_io, ResourcePaths::FONTS);

        let config = Config::load(&assets);

        if config.fullscreen {
            game_io.window_mut().set_fullscreen(true);
        }

        Self {
            config,
            global_save: GlobalSave::load(&assets),
            player_packages: PackageManager::new(PackageCategory::Player),
            card_packages: PackageManager::new(PackageCategory::Card),
            battle_packages: PackageManager::new(PackageCategory::Battle),
            block_packages: PackageManager::new(PackageCategory::Block),
            library_packages: PackageManager::new(PackageCategory::Library),
            battle_api: BattleLuaApi::new(),

            // sounds
            audio: AudioManager::new(),
            battle_music: assets.audio(ResourcePaths::BATTLE_MUSIC),
            cursor_move_sfx: assets.audio(ResourcePaths::CURSOR_MOVE_SFX),
            cursor_select_sfx: assets.audio(ResourcePaths::CURSOR_SELECT_SFX),
            cursor_cancel_sfx: assets.audio(ResourcePaths::CURSOR_CANCEL_SFX),
            cursor_error_sfx: assets.audio(ResourcePaths::CURSOR_ERROR_SFX),
            page_turn_sfx: assets.audio(ResourcePaths::PAGE_TURN_SFX),
            text_blip_sfx: assets.audio(ResourcePaths::TEXT_BLIP_SFX),
            warp_sfx: assets.audio(ResourcePaths::WARP_SFX),
            appear_sfx: assets.audio(ResourcePaths::APPEAR_SFX),
            card_select_open_sfx: assets.audio(ResourcePaths::CARD_SELECT_OPEN_SFX),
            card_select_confirm_sfx: assets.audio(ResourcePaths::CARD_SELECT_CONFIRM_SFX),
            turn_guage_sfx: assets.audio(ResourcePaths::TURN_GAUGE_SFX),
            attack_charging_sfx: assets.audio(ResourcePaths::ATTACK_CHARGING_SFX),
            attack_charged_sfx: assets.audio(ResourcePaths::ATTACK_CHARGED_SFX),
            player_deleted_sfx: assets.audio(ResourcePaths::PLAYER_DELETED_SFX),

            // assets
            font_texture,
            font_animator: Arc::new(Animator::load_new(&assets, ResourcePaths::FONTS_ANIMATION)),
            assets,

            // shaders
            sprite_pipeline: SpritePipeline::new(game_io, true),
            sprite_pipeline_collection: SpritePipelineCollection::new(game_io),
            background_pipeline: BackgroundPipeline::new(game_io),
            default_sampler: Sprite::new_sampler(game_io),
            background_sampler: TextureSampler::new(
                game_io,
                SamplingFilter::Nearest,
                EdgeSampling::Repeat,
            ),

            // networking
            network: Network::new(&args),
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
            PackageCategory::Block => {
                self.block_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::Card => {
                self.card_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::Battle | PackageCategory::Character => self
                .battle_packages
                .load_virtual_package(&self.assets, namespace, hash),
            PackageCategory::Library => {
                self.library_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
            PackageCategory::Player => {
                self.player_packages
                    .load_virtual_package(&self.assets, namespace, hash)
            }
        }?;

        // load child packages
        let child_packages: Vec<_> = package_info.child_packages().collect();
        let id = package_info.id.clone();

        self.library_packages
            .load_child_packages(&self.assets, namespace, child_packages);

        self.package_or_fallback_info(category, namespace, &id)
    }

    pub fn battle_dependencies(&self, props: &BattleProps) -> Vec<&PackageInfo> {
        let player_package_iter = props
            .player_setups
            .iter()
            .map(|setup| setup.player_package.package_info.triplet());

        let card_package_iter = props.player_setups.iter().flat_map(|setup| {
            let ns = if setup.local {
                PackageNamespace::Local
            } else {
                PackageNamespace::Remote(setup.index)
            };

            let card_iter = setup.folder.cards.iter();
            card_iter.map(move |card| (PackageCategory::Card, ns, card.package_id.clone()))
        });

        let battle_package_iter = std::iter::once(props.battle_package)
            .flatten()
            .map(|package| package.package_info().triplet());

        let package_iter = player_package_iter
            .chain(card_package_iter)
            .chain(battle_package_iter);

        self.package_dependency_iter(package_iter)
    }

    pub fn package_dependency_iter<I>(&self, iter: I) -> Vec<&PackageInfo>
    where
        I: IntoIterator<Item = (PackageCategory, PackageNamespace, String)>,
    {
        use std::collections::HashSet;
        let mut resolved = HashSet::new();

        let mut is_unresolved = move |triplet: &(PackageCategory, PackageNamespace, String)| {
            resolved.insert(triplet.clone())
        };

        let resolve = |(package_category, ns, id): (PackageCategory, PackageNamespace, String)| {
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
        id: &str,
    ) -> Option<&PackageInfo> {
        match category {
            PackageCategory::Character | PackageCategory::Library => self
                .library_packages
                .package_or_fallback(namespace, &id)
                .map(|package| package.package_info()),
            PackageCategory::Battle => self
                .battle_packages
                .package_or_fallback(namespace, &id)
                .map(|package| package.package_info()),
            PackageCategory::Block => self
                .block_packages
                .package_or_fallback(namespace, &id)
                .map(|package| package.package_info()),
            PackageCategory::Card => self
                .card_packages
                .package_or_fallback(namespace, &id)
                .map(|package| package.package_info()),
            PackageCategory::Player => self
                .player_packages
                .package_or_fallback(namespace, &id)
                .map(|package| package.package_info()),
        }
    }
}
