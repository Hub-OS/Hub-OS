use std::process::Command;

fn main() {
    fs_extra::dir::create_all("dist", true).unwrap();

    let scripts = ["dist-client", "dist-server", "dist-licenses"];

    for script in scripts {
        let build_output = Command::new("cargo")
            .args(["run", "--bin", script])
            .stdout(std::process::Stdio::inherit())
            .stderr(std::process::Stdio::inherit())
            .output()
            .unwrap();

        if !build_output.status.success() {
            // stdout + stderr are shared, no need to display anything
            return;
        }
    }
}
