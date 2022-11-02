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

mod args;
mod battle;
mod bindable;
mod ease;
mod lua_api;
mod overworld;
mod packages;
mod parse_util;
mod render;
mod resources;
mod saves;
mod scenes;
mod transitions;
mod zip;

use crate::args::Args;
use crate::resources::*;
use crate::scenes::BootScene;
use crate::scenes::Overlay;
use clap::Parser;
use framework::logging::*;
use framework::prelude::*;

pub fn main() -> anyhow::Result<()> {
    let args = Args::parse();

    let (log_sender, log_receiver) = flume::unbounded();
    default_logger::init_with_listener!(move |log| {
        let _ = log_sender.send(log);
    });

    let game = Game::new("Personal Terminal", TRUE_RESOLUTION.into(), |game_io| {
        Globals::new(game_io, args)
    })
    .with_resizable(true)
    .with_overlay(|game_io| Overlay::new(game_io));

    game.run(|game_io| BootScene::new(game_io, log_receiver))?;

    Ok(())
}

#[cfg_attr(target_os = "android", ndk_glue::main())]
pub fn android_main() {
    main().unwrap()
}
