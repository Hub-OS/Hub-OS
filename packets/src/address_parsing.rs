use std::net::{Ipv4Addr, SocketAddr};

const DEFAULT_PORT: u16 = 8765;
const DATA_SEPARATORS: [char; 3] = ['/', '?', '#'];

pub async fn resolve_socket_addr(mut address: &str) -> Option<SocketAddr> {
    address = strip_data(address);

    let port_index = address.find(':');

    let socket_addr = if address == "localhost" || address.starts_with("localhost:") {
        let port = match port_index {
            Some(index) => address[index + 1..].parse().ok()?,
            None => DEFAULT_PORT,
        };

        SocketAddr::new(Ipv4Addr::LOCALHOST.into(), port)
    } else {
        use async_std::net::ToSocketAddrs;
        let mut socket_addrs = address.to_socket_addrs().await.ok()?;

        let mut socket_addr = socket_addrs.next()?;

        if port_index.is_none() {
            socket_addr.set_port(DEFAULT_PORT);
        }

        socket_addr
    };

    Some(socket_addr)
}

pub fn strip_data(address: &str) -> &str {
    match address.find(DATA_SEPARATORS) {
        Some(index) => &address[..index],
        None => address,
    }
}

pub fn slice_data(address: &str) -> &str {
    match address.find(DATA_SEPARATORS) {
        Some(index) => &address[index..],
        None => "",
    }
}
