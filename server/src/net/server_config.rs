use crate::args::Args;

#[derive(Debug, Clone)]
pub struct ServerConfig {
    pub max_silence_duration: f32,
    pub heartbeat_rate: f32,
    pub args: Args,
}
