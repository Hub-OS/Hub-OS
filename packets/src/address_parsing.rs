use std::fmt::Write;
use std::net::{Ipv4Addr, SocketAddr};

pub const DEFAULT_PORT: u16 = 8765;
const DATA_SEPARATORS: [char; 3] = ['/', '?', '#'];

pub async fn resolve_socket_addr(mut address: &str) -> Option<SocketAddr> {
    address = strip_data(address);

    let port_index = address.find(':');
    let host = &address[..port_index.unwrap_or(address.len())];

    let port: u16 = match port_index {
        Some(index) => address[index + 1..].parse().ok()?,
        None => DEFAULT_PORT,
    };

    let socket_addr = if host == "localhost" {
        SocketAddr::new(Ipv4Addr::LOCALHOST.into(), port)
    } else {
        let socket_addrs = smol::net::resolve((host, port)).await.ok()?;

        let mut socket_addr = *socket_addrs.first()?;

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

pub fn uri_encode_raw(data: &[u8]) -> String {
    let mut encoded_string = String::with_capacity(data.len());

    for (i, &b) in data.iter().enumerate() {
        if b.is_ascii_alphanumeric() || b == b' ' || b == b'-' || b == b'_' || (b == b'.' && i > 0)
        {
            // doesn't need to be encoded
            encoded_string.push(b as char);
            continue;
        }

        // needs encoding
        write!(&mut encoded_string, "%{b:0>2X}").unwrap();
    }

    encoded_string
}

pub fn uri_encode(path: &str) -> String {
    uri_encode_raw(path.as_bytes())
}

pub fn uri_decode_raw(path: &str) -> Option<Vec<u8>> {
    let mut decoded_string = Vec::<u8>::with_capacity(path.len());

    let mut bytes = path.bytes().enumerate();

    while let Some((i, c)) = bytes.next() {
        if c != b'%' {
            // doesn't need to be decoded
            decoded_string.push(c);
            continue;
        }

        // needs decoding

        // skip two, also verifies that two characters exist for the next lines
        bytes.next()?;
        bytes.next()?;

        let b = u8::from_str_radix(&path[i + 1..i + 3], 16).ok()?;
        decoded_string.push(b);
    }

    Some(decoded_string)
}

pub fn uri_decode(path: &str) -> Option<String> {
    String::from_utf8(uri_decode_raw(path)?).ok()
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn uri_encoding() {
        const INPUT: &str = "a.b c-d_e:d?e%";
        const EXPECTED: &str = "a.b c-d_e%3Ad%3Fe%25";
        const ENCODED_MALFORMED: &str = "%";
        const BLANK: &str = "";

        assert_eq!(uri_encode(INPUT), EXPECTED);
        assert_eq!(uri_decode(EXPECTED), Some(INPUT.to_string()));
        assert_eq!(uri_decode(ENCODED_MALFORMED), None);
        assert_eq!(uri_decode(BLANK), Some(BLANK.to_string()));
    }

    #[test]
    fn euro() {
        const DECODED: &str = "€";
        const ENCODED: &str = "%E2%82%AC";

        assert_eq!(uri_encode(DECODED), ENCODED);
        assert_eq!(uri_decode(ENCODED), Some(String::from(DECODED)));
    }
}
