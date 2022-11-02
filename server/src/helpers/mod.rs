use std::net::{IpAddr, SocketAddr};

pub fn unwrap_and_parse_or_default<A>(option: Option<&str>) -> A
where
    A: Default + std::str::FromStr,
{
    option.unwrap_or_default().parse().unwrap_or_default()
}

pub fn normalize_path(path: &std::path::Path) -> std::path::PathBuf {
    let mut normalized_path: std::path::PathBuf = std::path::PathBuf::new();

    for component in path.components() {
        let component_as_os_str = component.as_os_str();

        if component_as_os_str == "." {
            continue;
        }

        if component_as_os_str == ".." {
            if normalized_path.file_name() == None {
                // file_name() returns none for ".." as last component and no component
                // this means we're building out the start like ../../../etc
                normalized_path.push("..");
            } else {
                normalized_path.pop();
            }
        } else {
            normalized_path.push(component);
        }
    }

    normalized_path
}

// use is_global when it's stabilized https://github.com/rust-lang/rust/issues/27709
pub fn is_internal_ip(ip: IpAddr) -> bool {
    match ip {
        IpAddr::V6(ipv6) => ipv6.is_loopback(),
        IpAddr::V4(ipv4) => {
            ipv4.is_private()
                || ipv4.is_loopback()
                || ipv4.is_link_local()
                || ipv4.is_broadcast()
                || ipv4.is_documentation()
        }
    }
}

/* \brief makes a localhost ip useable for pvp */
pub fn use_public_ip(address: SocketAddr, public_ip: IpAddr) -> SocketAddr {
    let ip = address.ip();

    if is_internal_ip(ip) {
        return std::net::SocketAddr::new(public_ip, address.port());
    }

    address
}

pub mod iterators;
