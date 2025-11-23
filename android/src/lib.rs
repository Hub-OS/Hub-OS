#![cfg(target_os = "android")]

use hub_os::framework::prelude::WinitPlatformApp;
use hub_os::{send_crash_report, ResourcePaths};
use packets::structures::FileHash;
use packets::zip;

mod network_locks;

#[no_mangle]
pub fn android_main(app: WinitPlatformApp) {
    std::panic::set_hook(Box::new(|p| {
        use std::backtrace::Backtrace;

        let backtrace = Backtrace::force_capture();
        let output = format!("{p}\n{backtrace}");
        eprintln!("{output}");
        send_crash_report(output);
    }));

    let locks = (
        network_locks::acquire_multicast_lock(&app),
        network_locks::acquire_low_latency_lock(&app),
    );

    // init_game_folders for set_current_dir
    ResourcePaths::init_game_folders(&app, None);

    std::env::set_current_dir(ResourcePaths::game_folder()).unwrap();

    update_resources(&app);

    hub_os::main(app).unwrap();

    std::mem::drop(locks);
}

fn update_resources(app: &WinitPlatformApp) {
    // instead of extracting a zip and running the exe,
    // users are only receiving an apk for android
    // so we're storing the resources as a zip in the apk and extracting when stored hashes differ
    const ZIP_FILE: &str = "resources.zip";
    const HASH_FILE: &str = "resources-hash";

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
