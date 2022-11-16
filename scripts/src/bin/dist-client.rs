use std::fs;
use std::process::{Command, ExitCode};

const BIN_NAME: &str = "real_pet";

fn main() -> ExitCode {
    let build_output = Command::new("cargo")
        .args(["build", "-p", BIN_NAME, "--release"])
        .stdout(std::process::Stdio::inherit())
        .stderr(std::process::Stdio::inherit())
        .output()
        .unwrap();

    if !build_output.status.success() {
        // stdout + stderr are shared, no need to display anything

        return ExitCode::FAILURE;
    }

    fs_extra::dir::create_all("dist/client", true).unwrap();

    // windows exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}.exe"),
        format!("dist/client/{BIN_NAME}.exe"),
    );

    // linux exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}"),
        format!("dist/client/{BIN_NAME}"),
    );

    fs_extra::dir::copy(
        "client/resources",
        "dist/client",
        &fs_extra::dir::CopyOptions::default(),
    )
    .unwrap();

    ExitCode::SUCCESS
}
