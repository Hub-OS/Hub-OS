// the project is wip, this allow should be removed at some point
#![allow(dead_code)]
// i prefer using new, and don't always need default (especially when there's no downstream)
#![allow(clippy::new_without_default)]
// i find this harder to read
#![allow(clippy::comparison_chain)]
// this can be worse for documentation, requires moving type info to a different location
#![allow(clippy::type_complexity)]
// false reports https://github.com/rust-lang/rust-clippy/issues/8148
#![allow(clippy::unnecessary_to_owned)]
#![allow(clippy::needless_collect)]
// it should be easy to read conditions, i don't want clever compression
#![allow(clippy::nonminimal_bool)]
// unit structs may eventually gain properties
#![allow(clippy::default_constructed_unit_structs)]

mod args;
mod battle;
mod bindable;
mod ease;
mod http;
mod lua_api;
mod memoize;
mod overlays;
mod overworld;
mod packages;
mod render;
mod resources;
mod saves;
mod scenes;
mod structures;
mod supporting_service;
mod transitions;

use crate::args::Args;
use crate::overlays::*;
use crate::render::PostProcessAdjust;
use crate::render::PostProcessColorBlindness;
use crate::render::PostProcessGhosting;
use crate::resources::*;
use crate::scenes::BootScene;
use clap::Parser;
use framework::logging::*;
use framework::prelude::*;
use rand::seq::SliceRandom;
use supporting_service::*;

const TITLE_LIST: [&str; 3] = [
    "Hub OS: Combat Network",
    "Hub OS: Combat Framework",
    "Hub OS: Modular Battler",
];

pub fn main(app: WinitPlatformApp) -> anyhow::Result<()> {
    let args = Args::parse();

    // init_game_folders in case we haven't already
    ResourcePaths::init_game_folders(&app, args.data_folder.clone());

    let (log_sender, log_receiver) = flume::unbounded();
    default_logger::init_with_listener!(move |log| {
        let _ = log_sender.send(log);
    });

    log::info!("Version {}", env!("CARGO_PKG_VERSION"));

    let random_title = TITLE_LIST.choose(&mut rand::thread_rng()).unwrap();
    let game = Game::<WinitGameLoop>::new(random_title, (RESOLUTION * 4).into())
        .with_platform_app(app)
        .with_resizable(true)
        .with_setup(|game_io| {
            let globals = Globals::new(game_io, args);
            game_io.set_resource(globals);
        })
        .with_service(SupportingService::new)
        .with_post_process(|game_io| PostProcessGhosting::new(game_io))
        .with_post_process(|game_io| PostProcessAdjust::new(game_io))
        .with_post_process(|game_io| PostProcessColorBlindness::new(game_io))
        .with_overlay(GameOverlayTarget::Render, |game_io| {
            DebugOverlay::new(game_io)
        });

    #[cfg(target_os = "android")]
    let game = game.with_overlay(GameOverlayTarget::Window, |game_io| {
        VirtualController::new(game_io)
    });

    game.run(|game_io| BootScene::new(game_io, log_receiver))?;

    Ok(())
}

#[cfg(target_os = "android")]
#[no_mangle]
pub fn android_main(app: WinitPlatformApp) {
    // init_game_folders for set_current_dir
    ResourcePaths::init_game_folders(&app, None);

    std::env::set_current_dir(ResourcePaths::game_folder()).unwrap();

    update_resources(&app);

    main(app).unwrap()
}

#[cfg(target_os = "android")]
fn update_resources(app: &WinitPlatformApp) {
    // instead of extracting a zip and running the exe,
    // users are only receiving an apk for android
    // so we're storing the resources as a zip in the apk and extracting when stored hashes differ
    const ZIP_FILE: &str = "resources.zip";
    const HASH_FILE: &str = "resources-hash";

    use packets::structures::FileHash;
    use packets::zip;
    use std::ffi::CString;

    // find the zip
    let zip_c_name = CString::new(ZIP_FILE).unwrap();

    let ndk_assets = app.asset_manager();
    let mut zip_asset = ndk_assets.open(zip_c_name.as_c_str()).unwrap();
    let zip_bytes = zip_asset.buffer().unwrap();

    // compare hashes
    let resources_hash = FileHash::hash(zip_bytes);
    let resources_match = std::fs::read(HASH_FILE)
        .map(|bytes| resources_hash.as_bytes() == bytes)
        .unwrap_or_default();

    if resources_match {
        return;
    }

    // delete old resources
    let _ = std::fs::remove_dir_all("resources");

    // extract zip
    zip::extract(zip_bytes, |path, mut file| {
        use std::io::Read;

        if !file.is_file() {
            return;
        }

        let mut bytes = Vec::new();
        file.read_to_end(&mut bytes).unwrap();

        if let Some(parent_path) = ResourcePaths::parent(&path) {
            std::fs::create_dir_all(parent_path).unwrap();
        }

        std::fs::write(path, &bytes).unwrap();
    });

    // update stored hash
    std::fs::write("resources-hash", resources_hash.as_bytes()).unwrap();
}
