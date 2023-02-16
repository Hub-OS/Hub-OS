use std::process::{Command, ExitCode};

fn main() -> ExitCode {
    let android_assets_path = "cache/android_assets";
    let zip_path = String::from(android_assets_path) + "/resources.zip";

    std::fs::create_dir_all(android_assets_path).unwrap();

    // linux only for now
    let zip_output = Command::new("zip")
        .args([&zip_path, "resources", "-r", "-D"])
        .current_dir("client")
        .stdout(std::process::Stdio::inherit())
        .stderr(std::process::Stdio::inherit())
        .output()
        .unwrap();

    if zip_output.status.success() {
        ExitCode::SUCCESS
    } else {
        // stdout + stderr are shared, no need to display anything
        ExitCode::FAILURE
    }
}
