// #![windows_subsystem = "windows"]

use clap::Parser;
use hub_os::{Args, PlatformApp, ResourcePaths, ResourcePathsOptions};

fn main() {
    std::panic::set_hook(Box::new(|p| {
        use std::backtrace::Backtrace;

        let backtrace = Backtrace::force_capture();
        let output = format!("{p}\n{backtrace}");

        let _ = std::fs::write("crash.txt", &output);
        hub_os::crash_reports::print_crash_report(output.clone());
        hub_os::crash_reports::send_crash_report(output);
    }));

    let args = Args::parse();
    let game_path = resolve_game_path();
    let data_path = resolve_data_path(&game_path, &args);

    let resource_options = ResourcePathsOptions {
        game_path,
        data_path,
    };

    // check lib.rs
    hub_os::main(PlatformApp::default(), args, resource_options).unwrap()
}

fn resolve_game_path() -> String {
    let game_path = std::env::current_dir().unwrap_or_default();
    ResourcePaths::clean_folder(&game_path.to_string_lossy())
}

fn resolve_data_path(game_path: &str, args: &Args) -> String {
    if let Some(path) = &args.data_folder {
        // use folder from arg

        let path = match std::path::absolute(path) {
            Ok(path) => path,
            Err(err) => {
                panic!("Invalid data folder: {err:?}");
            }
        };

        ResourcePaths::clean_folder(&path.to_string_lossy())
    } else {
        // use shared folder
        let shared_path = if cfg!(target_os = "windows") {
            dirs_next::document_dir().map(|d| d.join("My Games"))
        } else {
            dirs_next::data_dir()
        };

        if let Some(path) = shared_path {
            // canonicalize to capture the existing capitalization
            let path = std::fs::canonicalize(&path).unwrap_or(path);
            let path = path.join("Hub OS");
            let data_path = ResourcePaths::clean_folder(&path.to_string_lossy());

            let _ = std::fs::create_dir_all(&data_path);

            data_path
        } else {
            // use game folder
            game_path.to_string()
        }
    }
}
