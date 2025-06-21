use crate::resources::{ResourcePaths, IDENTITY_LEN};
use packets::address_parsing::uri_encode;
use rand::{rngs::OsRng, RngCore};
use std::io::Write;

pub struct Identity {
    data: Vec<u8>,
}

impl Identity {
    pub fn for_address(address: &str) -> Self {
        let address = packets::address_parsing::strip_data(address).replace(':', "_p");
        let address = uri_encode(&address);

        let folder = ResourcePaths::identity_folder();
        let file_path = folder.clone() + &address;

        let data = std::fs::read(&file_path).unwrap_or_else(|_| {
            const HEADER: &[u8] = b"random:";

            let mut data = vec![0; IDENTITY_LEN + HEADER.len()];

            // write header
            let _ = (&mut data[0..HEADER.len()]).write(HEADER);

            // fill the rest with random bytes
            OsRng.fill_bytes(&mut data[HEADER.len()..]);

            let _ = std::fs::create_dir_all(&folder);

            if let Err(e) = std::fs::write(file_path, &data) {
                log::error!("{e}");
            }

            data
        });

        Self { data }
    }

    pub fn data(&self) -> &[u8] {
        &self.data
    }
}
