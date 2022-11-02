use std::fs;
use std::process::Command;

fn main() {
    // licenses
    let cargo_about_output = Command::new("cargo")
        .args(["about", "generate", "scripts/about.hbs"])
        .stderr(std::process::Stdio::inherit())
        .output()
        .unwrap();

    if !cargo_about_output.status.success() {
        // stdout + stderr are shared, no need to display anything
        return;
    }

    fs::create_dir("dist").unwrap();
    let _ = fs::write("dist/third_party_licenses.html", &cargo_about_output.stdout);
}
