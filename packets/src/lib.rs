use std::time::Duration;

pub const VERSION_ID: &str = "https://github.com/ArthurCose/RealPET";
pub const VERSION_ITERATION: u64 = 38;
pub const SERVER_TICK_RATE_F: f32 = 1.0 / 20.0; // 1 / 20 of a second
pub const SERVER_TICK_RATE: Duration = Duration::from_millis(50); // 1 / 20 of a second
pub const MAX_IDLE_DURATION: Duration = Duration::from_secs(1);

mod client_packets;
mod netplay_packets;
mod packet_channels;
mod server_comm_packets;
mod server_packets;

pub mod address_parsing;
pub mod lua_helpers;
pub mod structures;
pub mod zip;
pub use client_packets::*;
pub use netplay_packets::*;
pub use network_channels::*;
pub use packet_channels::*;
pub use server_comm_packets::*;
pub use server_packets::*;
