#[derive(Clone)]
pub struct ServerConfig {
    pub public_ip: std::net::IpAddr,
    pub port: u16,
    pub log_connections: bool,
    pub log_packets: bool,
    pub max_payload_size: u16,
    pub resend_budget: usize,
    pub receiving_drop_rate: f32,
    pub player_asset_limit: usize,
    pub avatar_dimensions_limit: u32,
    pub custom_emotes_path: Option<String>,
    pub max_idle_packet_duration: f32,
    pub max_silence_duration: f32,
    pub heartbeat_rate: f32,
}
