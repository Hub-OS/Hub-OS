use std::net::{Ipv4Addr, SocketAddr, SocketAddrV4};
use std::time::Duration;

pub const VERSION_ID: &str = "Hub OS";
pub const VERSION_ITERATION: u64 = 105;
pub const SERVER_TICK_RATE_F: f32 = 1.0 / 20.0; // 1 / 20 of a second
pub const SERVER_TICK_RATE: Duration = Duration::from_millis(50); // 1 / 20 of a second
pub const MAX_IDLE_DURATION: Duration = Duration::from_secs(1);
/// Allow temporary packet loss without discarding the reliable connection.
pub const CONNECTION_TIMEOUT: Duration = Duration::from_secs(20);
pub const MIN_DISCONNECT_GRACE_TIME: u8 = 5;
pub const MAX_DISCONNECT_GRACE_TIME: u8 = 120;

pub fn disconnect_grace_duration(seconds: u8) -> Duration {
    Duration::from_secs(seconds.clamp(MIN_DISCONNECT_GRACE_TIME, MAX_DISCONNECT_GRACE_TIME) as u64)
}

pub const MULTICAST_V4: Ipv4Addr = Ipv4Addr::new(224, 0, 2, 88);
pub const MULTICAST_PORT: u16 = 7878;
pub const MULTICAST_ADDR: SocketAddr =
    SocketAddr::V4(SocketAddrV4::new(MULTICAST_V4, MULTICAST_PORT));

mod client_packets;
mod multicast_packets;
mod netplay_packets;
mod packet_channels;
mod server_comm_packets;
mod server_packets;
mod sync_data_packets;

pub mod address_parsing;
pub mod lua_helpers;
pub mod structures;
pub mod zip;
pub use client_packets::*;
pub use multicast_packets::*;
pub use netplay_packets::*;
pub use network_channels::*;
pub use packet_channels::*;
pub use server_comm_packets::*;
pub use server_packets::*;
pub use sync_data_packets::*;
