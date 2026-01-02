use instant::Duration;

#[derive(Clone)]
pub struct Config {
    pub mtu: u16,
    pub initial_bytes_per_second: usize,
    pub max_bytes_per_second: Option<usize>,
    pub bytes_per_second_increase_factor: f32,
    pub bytes_per_second_slow_factor: f32,
    pub rtt_resend_factor: f32,
    pub initial_rtt: Duration,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            mtu: 1400,
            initial_bytes_per_second: 1024 * 128, // start at 1/8 MBps or 1 Mbps
            max_bytes_per_second: None,
            bytes_per_second_increase_factor: 1.25,
            bytes_per_second_slow_factor: 0.5,
            rtt_resend_factor: 1.0,
            initial_rtt: Duration::from_millis(500),
        }
    }
}
