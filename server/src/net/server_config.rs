use crate::args::Args;

#[derive(Debug, Clone)]
pub struct ServerConfig {
    pub public_ip: std::net::IpAddr,
    pub max_silence_duration: f32,
    pub heartbeat_rate: f32,
    pub args: Args,
}
