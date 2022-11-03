use std::fs;
use std::process::{Command, ExitCode};

fn main() -> ExitCode {
    // licenses
    let cargo_about_output = Command::new("cargo")
        .args(["about", "generate", "scripts/about.hbs"])
        .stderr(std::process::Stdio::inherit())
        .output()
        .unwrap();

    if !cargo_about_output.status.success() {
        // stdout + stderr are shared, no need to display anything
        return ExitCode::FAILURE;
    }

    let _ = fs::create_dir("dist");
    let _ = fs::write("dist/third_party_licenses.html", &cargo_about_output.stdout);

    ExitCode::SUCCESS
}
