use clap::Parser;

#[derive(Parser, Debug, Clone)]
pub struct Args {
    #[arg(
        short,
        long,
        default_value = "8765",
        value_parser = clap::value_parser!(u16).range(1..)
    )]
    pub port: u16,

    /// Logs connects and disconnects
    #[arg(long)]
    pub log_connections: bool,

    /// Logs received packets (useful for debugging)
    #[arg(long)]
    pub log_packets: bool,

    /// Maximum data size a packet can carry, excluding UDP headers (reduce for lower packet drop rate)
    #[arg(
        long,
        value_name = "SIZE_IN_BYTES",
        default_value = "1400",
        value_parser = clap::value_parser!(u16).range(100..=10240)
    )]
    pub max_payload_size: u16,

    /// Budget of bytes each client has for the server to spend on resending packets
    #[arg(long, value_name = "SIZE_IN_BYTES", default_value = "65536")]
    pub resend_budget: usize,

    /// Rate of received packets to randomly drop for simulating an unstable connection
    #[arg(
        long,
        value_name = "PERCENTAGE",
        default_value = "0.0",
        value_parser = clap::builder::ValueParser::new(percentage_parser)
    )]
    pub receiving_drop_rate: f32,

    /// Sets the file size limit for avatar files bytes
    #[arg(
      long,
      value_name = "SIZE_IN_KiB",
      default_value = "50",
      value_parser = clap::builder::ValueParser::new(kib_to_bytes_parser),
      help = "Sets the file size limit for avatar files (in KiB)"
    )]
    pub player_asset_limit: usize,

    /// Sets the limit for dimensions of a single avatar frame
    #[arg(long, value_name = "SIDE_LENGTH", default_value = "80")]
    pub avatar_dimensions_limit: u32,

    #[arg(long, value_name = "ASSET_PATH", 
    value_parser = clap::builder::ValueParser::new(optional_asset_path_parser))]
    pub emotes_animation_path: Option<String>,

    #[arg(long, value_name = "ASSET_PATH", 
    value_parser = clap::builder::ValueParser::new(optional_asset_path_parser))]
    pub emotes_texture_path: Option<String>,
}

fn percentage_parser(value: &str) -> Result<f32, String> {
    let error_message = "PERCENTAGE must be between 0.0 and 100.0";

    let percentage = value
        .parse::<f32>()
        .map_err(|_| String::from(error_message))?;

    if !(0.0..=100.0).contains(&percentage) {
        Err(String::from(error_message))
    } else {
        Ok(percentage)
    }
}

fn kib_to_bytes_parser(value: &str) -> Result<usize, String> {
    let error_message = "SIZE_IN_KiB must be a positive number";

    let kib = value
        .parse::<usize>()
        .map_err(|_| String::from(error_message))?;

    Ok(kib * 1024)
}

fn optional_asset_path_parser(value: &str) -> Result<Option<String>, String> {
    if value.starts_with("/server/assets/") {
        Ok(Some(value.to_string()))
    } else {
        Err(String::from(
            "ASSET_PATH must start with \"/server/assets/\"",
        ))
    }
}
