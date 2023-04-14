#![windows_subsystem = "windows"]

use std::panic;

fn main() {
    panic::set_hook(Box::new(|p| {
        use std::backtrace::Backtrace;

        let backtrace = Backtrace::force_capture();
        let output = format!("{p}\n{backtrace}");

        let _ = std::fs::write("crash.txt", &output);
        log::error!("{output}");
    }));

    // check lib.rs
    real_pet::main().unwrap()
}
