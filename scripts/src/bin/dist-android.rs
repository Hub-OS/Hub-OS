use std::fs;
use std::process::{Command, ExitCode};

const BIN_NAME: &str = "hub_os";

fn main() -> ExitCode {
    let zip_output = Command::new("cargo")
        .args(["run", "--bin", "generate-android-assets"])
        .stdout(std::process::Stdio::inherit())
        .stderr(std::process::Stdio::inherit())
        .output()
        .unwrap();

    if !zip_output.status.success() {
        // stdout + stderr are shared, no need to display anything
        return ExitCode::FAILURE;
    }

    let build_output = Command::new("cargo")
        .args(["apk", "build", "--lib", "--release"])
        .current_dir("./client")
        .stdout(std::process::Stdio::inherit())
        .stderr(std::process::Stdio::inherit())
        .output()
        .unwrap();

    if !build_output.status.success() {
        // stdout + stderr are shared, no need to display anything
        return ExitCode::FAILURE;
    }

    fs_extra::dir::create_all("dist", false).unwrap();

    let _ = fs::copy(
        format!("target/release/apk/{BIN_NAME}.apk"),
        format!("dist/{BIN_NAME}.apk"),
    );

    ExitCode::SUCCESS
}
