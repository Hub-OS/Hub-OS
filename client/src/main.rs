use std::panic;

fn main() {
    panic::set_hook(Box::new(|p| {
        let output = format!("{p}");
        let _ = std::fs::write("crash.txt", &output);
        log::error!("{p}");
    }));

    // check lib.rs
    real_pet::main().unwrap()
}
