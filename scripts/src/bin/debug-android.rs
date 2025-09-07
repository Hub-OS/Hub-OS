use std::process::{Command, ExitCode};

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
        .args(["apk", "run", "--lib"])
        .current_dir("./android")
        .stdout(std::process::Stdio::inherit())
        .stderr(std::process::Stdio::inherit())
        .output()
        .unwrap();

    if !build_output.status.success() {
        // stdout + stderr are shared, no need to display anything
        return ExitCode::FAILURE;
    }

    ExitCode::SUCCESS
}
