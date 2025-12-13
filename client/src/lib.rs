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
mod tips;
mod transitions;

pub mod crash_reports;

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
use rand::seq::IndexedRandom;
use supporting_service::*;

// exported for the android crate
pub use framework;
pub use resources::ResourcePaths;

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

    default_logger::DefaultLogger::new()
        .with_global_level_filter(log::LevelFilter::Warn)
        .with_crate_level_filter(env!("CARGO_PKG_NAME"), log::LevelFilter::Trace)
        .with_crate_level_filter("framework", log::LevelFilter::Trace)
        .with_listener(move |log| {
            let _ = log_sender.send(log);
        })
        .init()
        .unwrap();

    log::info!("Version {}", env!("CARGO_PKG_VERSION"));

    let random_title = TITLE_LIST.choose(&mut rand::rng()).unwrap();
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
