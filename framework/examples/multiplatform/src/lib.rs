mod main_scene;
use main_scene::MainScene;

use framework::logging::*;
use framework::prelude::Game;

pub fn shared_main() -> anyhow::Result<()> {
    default_logger::init!();

    let game = Game::new("Sprites", (800, 600), |_| ());

    game.run(|game_io| MainScene::new(game_io))
}

#[cfg(target_arch = "wasm32")]
use wasm_bindgen::prelude::*;

#[cfg_attr(target_arch = "wasm32", wasm_bindgen(start))]
#[cfg_attr(target_os = "android", ndk_glue::main(backtrace = "on"))]
pub fn sync_main() {
    shared_main().unwrap();
}
