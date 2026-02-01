use std::fs;
use std::process::{Command, ExitCode};

const BIN_NAME: &str = "hub_os_server";

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

    // areas
    fs_extra::dir::create_all("dist/server", true).unwrap();

    for folder in ["areas", "assets", "mods", "scripts"] {
        fs_extra::dir::copy(
            format!("server/{folder}"),
            "dist/server",
            &fs_extra::dir::CopyOptions::default(),
        )
        .unwrap();
    }

    // windows exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}.exe"),
        format!("dist/server/{BIN_NAME}.exe"),
    );

    // linux + mac exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}"),
        format!("dist/server/{BIN_NAME}"),
    );

    // mac x86_64 exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}-x86_64"),
        format!("dist/server/{BIN_NAME}-x86_64"),
    );

    // mac ARM exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}-aarch64"),
        format!("dist/server/{BIN_NAME}-aarch64"),
    );

    ExitCode::SUCCESS
}
