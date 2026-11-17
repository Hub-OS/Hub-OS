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
        log_path: Some(game_path.clone() + "log.txt"),
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

const DEFAULT_DATA_DIR_NAME: &str = "Hub OS";

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
            resolve_and_migrate_windows_data_dir()
        } else {
            dirs_next::data_dir()
        };

        if let Some(path) = shared_path {
            // canonicalize to capture the existing capitalization
            let mut path = std::fs::canonicalize(&path).unwrap_or(path);
            path.push(DEFAULT_DATA_DIR_NAME);

            let data_path = ResourcePaths::clean_folder(&path.to_string_lossy());

            let _ = std::fs::create_dir_all(&data_path);

            data_path
        } else {
            // use game folder
            game_path.to_string()
        }
    }
}

//  returns the parent folder, migrates the data folder if it's found in an old location
fn resolve_and_migrate_windows_data_dir() -> Option<std::path::PathBuf> {
    let mut saved_games_path = dirs_next::home_dir()?;
    saved_games_path.push("Saved Games");

    if let Some(mut my_games_path) = dirs_next::document_dir() {
        my_games_path.push("My Games");

        let full_my_games_path = my_games_path.join(DEFAULT_DATA_DIR_NAME);

        // making a copy with the folder name appended
        let full_saved_games_path = saved_games_path.join(DEFAULT_DATA_DIR_NAME);

        // migrate old data
        let migrate_result = std::fs::rename(&full_my_games_path, &full_saved_games_path);

        // see if migration failed, if it did, just use the old path
        let exists = |p| std::fs::exists(p).is_ok_and(|v| v);

        if migrate_result.is_err() && exists(full_my_games_path) && !exists(full_saved_games_path) {
            return Some(my_games_path);
        }
    }

    Some(saved_games_path)
}
