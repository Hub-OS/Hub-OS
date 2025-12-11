mod args;
mod helpers;
mod jobs;
mod logger;
mod net;
mod plugins;
mod threads;

use clap::Parser;
use plugins::LuaPluginInterface;

fn main() {
    std::panic::set_hook(Box::new(|p| {
        use std::backtrace::Backtrace;

        let backtrace = Backtrace::force_capture();
        let output = format!("{p}\n{backtrace}");

        let _ = std::fs::write("crash.txt", &output);
        log::error!("{output}");
    }));

    logger::init();

    let args = args::Args::parse();

    let config = net::ServerConfig {
        max_silence_duration: 5.0,
        heartbeat_rate: 0.5,
        args,
    };

    let future = net::ServerBuilder::new(config)
        .with_plugin_interface(Box::new(LuaPluginInterface::new()))
        .start();

    smol::block_on(async move {
        if let Err(err) = future.await {
            panic!("{}", err);
        }
    });
}
