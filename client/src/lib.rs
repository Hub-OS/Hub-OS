// the project is wip, this allow should be removed at some point
#![allow(dead_code)]

mod args;
mod battle;
mod bindable;
mod ease;
mod lua_api;
mod memoize;
mod overlays;
mod overworld;
mod packages;
mod render;
mod requests;
mod resources;
mod saves;
mod scenes;
mod stopwatch;
mod structures;
mod supporting_service;
mod tips;
mod transitions;

pub mod crash_reports;

use crate::args::Args;
use crate::resources::*;
use crate::scenes::BootStage1;
use clap::Parser;
use framework::logging::*;
use framework::prelude::*;
use framework::runtime::GameWindowLoop;
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

#[cfg(target_os = "android")]
pub type GameLoop = framework::prelude::AndroidGameLoop;
#[cfg(not(target_os = "android"))]
pub type GameLoop = framework::prelude::WinitGameLoop;

pub type PlatformApp = <GameLoop as GameWindowLoop>::PlatformApp;

pub fn main(app: PlatformApp) -> anyhow::Result<()> {
    let args = Args::parse();

    // init_game_folders in case we haven't already
    ResourcePaths::init_game_folders(&app, args.data_folder.clone());

    let (log_sender, log_receiver) = flume::unbounded();

    let logger_result = default_logger::DefaultLogger::new()
        .with_global_level_filter(log::LevelFilter::Warn)
        .with_crate_level_filter(env!("CARGO_PKG_NAME"), log::LevelFilter::Trace)
        .with_crate_level_filter("framework", log::LevelFilter::Trace)
        .with_crate_level_filter("gilrs", log::LevelFilter::Error) // warns about mapping when no controller is connected
        .with_listener(move |log| {
            let _ = log_sender.send(log);
        })
        .init();

    if logger_result.is_err() {
        log::error!("Logger already set");
    }

    log::info!("Version {}", env!("CARGO_PKG_VERSION"));

    let random_title = TITLE_LIST.choose(&mut rand::rng()).unwrap();
    Game::<GameLoop>::new(random_title, (RESOLUTION * 4).into())
        .with_platform_app(app)
        .with_resizable(true)
        .run(|game_io| BootStage1::new(game_io, args, log_receiver))?;

    Ok(())
}
