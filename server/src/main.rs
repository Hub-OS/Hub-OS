mod helpers;
mod jobs;
mod logger;
mod net;
mod plugins;
mod threads;

use helpers::unwrap_and_parse_or_default;
use plugins::LuaPluginInterface;
use std::net::IpAddr;

#[async_std::main]
async fn main() {
    std::panic::set_hook(Box::new(|p| {
        let output = format!("{p}");
        let _ = std::fs::write("crash.txt", &output);
    }));

    logger::init();

    let matches = clap::Command::new("OpenNetBattle Server")
    .arg(
      clap::Arg::new("port")
        .short('p')
        .long("port")
        .value_name("PORT")
        .default_value("8765")
        .takes_value(true)
        .validator(|value| {
          let error_message = "PORT must be > 0 and < 65535";
          let port = value
            .parse::<u16>()
            .map_err(|_| String::from(error_message))?;

          if port != 0 {
            Ok(())
          } else {
            Err(String::from(error_message))
          }
        }),
    )
    .arg(
      clap::Arg::new("log_connections")
        .long("log-connections")
        .help("Logs connects and disconnects"),
    )
    .arg(
      clap::Arg::new("log_packets")
        .long("log-packets")
        .help("Logs received packets (useful for debugging)"),
    )
    .arg(
      clap::Arg::new("max_payload_size")
        .long("max-payload-size")
        .help("Maximum data size a packet can carry, excluding UDP headers (reduce for lower packet drop rate)")
        .value_name("SIZE_IN_BYTES")
        .default_value("1400")
        .takes_value(true)
        .validator(|value| {
          let error_message = "Invalid payload size";
          let max_payload_size = value
            .parse::<u16>()
            .map_err(|_| String::from(error_message))?;

          // max size defined by NetPlayConfig::MAX_BUFFER_LEN
          if (100..=10240).contains(&max_payload_size) {
            Ok(())
          } else {
            Err(String::from(error_message))
          }
        }),
    )
    .arg(
      clap::Arg::new("resend_budget")
        .long("resend-budget")
        .help("Budget of bytes each client has for the server to spend on resending packets")
        .value_name("SIZE_IN_BYTES")
        .default_value("65536") // nearest power of a power of two to (test data / 2 skips / 2 for safety / 2 reliability types)
        .takes_value(true)
        .validator(|value| {
          let error_message = "Invalid size";

          let resend_budget = value.parse::<isize>().map_err(|_| String::from(error_message))?;

          if resend_budget < 0 {
            Err(String::from(error_message))
          } else {
            Ok(())
          }
        }),
    )
    .arg(
      clap::Arg::new("receiving_drop_rate")
        .long("receiving-drop-rate")
        .help("Rate of received packets to randomly drop for simulating an unstable connection")
        .value_name("PERCENTAGE")
        .default_value("0.0")
        .takes_value(true)
        .validator(|value| {
          let error_message = "PERCENTAGE must be between 0.0 and 100.0";

          let resend_budget = value.parse::<f32>().map_err(|_| String::from(error_message))?;

          if !(0.0..=100.0).contains(&resend_budget) {
            Err(String::from(error_message))
          } else {
            Ok(())
          }
        }),
    )
    .arg(
      clap::Arg::new("player_asset_limit")
        .long("player-asset-limit")
        .help("Sets the file size limit for avatar files (in KiB)")
        .value_name("SIZE_IN_KiB")
        .default_value("50")
        .takes_value(true)
        .validator(|value| match value.parse::<usize>() {
          Ok(_) => Ok(()),
          Err(_) => Err(String::from("Invalid file size")),
        }),
    )
    .arg(
      clap::Arg::new("avatar_dimensions_limit")
        .long("avatar-dimensions-limit")
        .help("Sets the limit for dimensions of a single avatar frame")
        .value_name("SIDE_LENGTH")
        .default_value("80")
        .takes_value(true)
        .validator(|value| match value.parse::<u32>() {
          Ok(_) => Ok(()),
          Err(_) => Err(String::from("Invalid length")),
        }),
    )
    .arg(
      clap::Arg::new("custom_emotes_path")
        .long("custom-emotes-path")
        .value_name("ASSET_PATH")
        .validator(|value| {
          if value.starts_with("/server/assets/") {
            Ok(())
          } else {
            Err(String::from("ASSET_PATH must start with \"/server/assets/\""))
          }
        }),
    )
    .get_matches();

    let config = net::ServerConfig {
        public_ip: get_public_ip().unwrap_or_else(|_| IpAddr::from([127, 0, 0, 1])), // default to localhost
        // validators makes these safe to unwrap
        port: matches.value_of("port").unwrap().parse().unwrap(),
        log_connections: matches.is_present("log_connections"),
        log_packets: matches.is_present("log_packets"),
        max_payload_size: unwrap_and_parse_or_default(matches.value_of("max_payload_size")),
        resend_budget: matches.value_of("resend_budget").unwrap().parse().unwrap(),
        receiving_drop_rate: matches
            .value_of("receiving_drop_rate")
            .unwrap()
            .parse()
            .unwrap(),
        player_asset_limit: unwrap_and_parse_or_default::<usize>(
            matches.value_of("player_asset_limit"),
        ) * 1024,
        avatar_dimensions_limit: unwrap_and_parse_or_default(
            matches.value_of("avatar_dimensions_limit"),
        ),
        custom_emotes_path: matches
            .value_of("custom_emotes_path")
            .map(|path| path.to_string()),
        max_idle_packet_duration: 1.0,
        max_silence_duration: 5.0,
        heartbeat_rate: 0.5,
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
