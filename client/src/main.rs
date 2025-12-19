// #![windows_subsystem = "windows"]

use hub_os::PlatformApp;

fn main() {
    std::panic::set_hook(Box::new(|p| {
        use std::backtrace::Backtrace;

        let backtrace = Backtrace::force_capture();
        let output = format!("{p}\n{backtrace}");

        let _ = std::fs::write("crash.txt", &output);
        hub_os::crash_reports::print_crash_report(output.clone());
        hub_os::crash_reports::send_crash_report(output);
    }));

    // check lib.rs
    #[allow(clippy::unit_arg)]
    hub_os::main(PlatformApp::default()).unwrap()
}
