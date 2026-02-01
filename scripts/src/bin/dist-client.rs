use std::fs;
use std::process::{Command, ExitCode};

const BIN_NAME: &str = "hub_os";

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

    let _ = fs::create_dir_all("dist");

    // windows exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}.exe"),
        format!("dist/{BIN_NAME}.exe"),
    );

    // linux + mac exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}"),
        format!("dist/{BIN_NAME}"),
    );

    // mac x86_64 exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}-x86_64"),
        format!("dist/{BIN_NAME}-x86_64"),
    );

    // mac ARM exe
    let _ = fs::copy(
        format!("target/release/{BIN_NAME}-aarch64"),
        format!("dist/{BIN_NAME}-aarch64"),
    );

    let _ = fs::remove_dir_all("dist/resources");
    fs_extra::dir::copy("client/resources", "dist", &Default::default()).unwrap();

    ExitCode::SUCCESS
}
