use std::time::Duration;

pub const VERSION_ID: &str = "https://github.com/ArthurCose/RealPET";
pub const VERSION_ITERATION: u64 = 5;
pub const SERVER_TICK_RATE: Duration = Duration::from_millis(50); // 1 / 20 of a second

mod client_packets;
mod netplay_packets;
mod packet_channels;
mod server_comm_packets;
mod server_packets;

pub mod address_parsing;
pub mod structures;
pub use client_packets::*;
pub use netplay_packets::*;
pub use network_channels::*;
pub use packet_channels::*;
pub use server_comm_packets::*;
pub use server_packets::*;
