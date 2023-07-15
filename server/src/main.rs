mod args;
mod helpers;
mod jobs;
mod logger;
mod net;
mod plugins;
mod threads;

use clap::Parser;
use plugins::LuaPluginInterface;
use std::net::IpAddr;

#[async_std::main]
async fn main() {
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
        public_ip: get_public_ip().unwrap_or_else(|_| IpAddr::from([127, 0, 0, 1])),
        max_idle_packet_duration: 1.0,
        max_silence_duration: 5.0,
        heartbeat_rate: 0.5,
        args,
    };

    let future = net::ServerBuilder::new(config)
        .with_plugin_interface(Box::new(LuaPluginInterface::new()))
        .start();

    if let Err(err) = future.await {
        panic!("{}", err);
    }
}

fn get_public_ip() -> Result<IpAddr, Box<dyn std::error::Error>> {
    use isahc::config::{Configurable, RedirectPolicy};
    use isahc::Request;
    use std::io::Read;
    use std::str::FromStr;

    let request = Request::get("http://checkip.amazonaws.com")
        .redirect_policy(RedirectPolicy::Follow)
        .body(())?;

    let mut response = isahc::send(request)?;
    let mut response_text = String::new();
    response.body_mut().read_to_string(&mut response_text)?;

    let ip_string = response_text.replace('\n', "");

    Ok(IpAddr::from_str(&ip_string)?)
}
