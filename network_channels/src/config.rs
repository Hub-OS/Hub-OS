use instant::Duration;

#[derive(Clone)]
pub struct Config {
    pub mtu: u16,
    pub bytes_per_tick: usize,
    pub rtt_resend_factor: f32,
    pub initial_rtt: Duration,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            mtu: 1400,
            rtt_resend_factor: 0.5,
            bytes_per_tick: 65536,
            initial_rtt: Duration::from_millis(500),
        }
    }
}
