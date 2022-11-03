use std::fs;
use std::process::Command;

const CLIENT_NAME: &str = "real_pet";

fn main() {
    let build_output = Command::new("cargo")
        .args(["build", "-p", CLIENT_NAME, "--release"])
        .stdout(std::process::Stdio::inherit())
        .stderr(std::process::Stdio::inherit())
        .output()
        .unwrap();

    if !build_output.status.success() {
        // stdout + stderr are shared, no need to display anything
        return;
    }

    fs_extra::dir::create_all("dist/client", true).unwrap();

    // windows exe
    let _ = fs::copy(
        format!("target/release/{CLIENT_NAME}.exe"),
        format!("dist/client/{CLIENT_NAME}.exe"),
    );

    // linux exe
    let _ = fs::copy(
        format!("target/release/{CLIENT_NAME}"),
        format!("dist/client/{CLIENT_NAME}"),
    );

    fs_extra::dir::copy(
        "client/resources",
        "dist/client",
        &fs_extra::dir::CopyOptions::default(),
    )
    .unwrap();
}
