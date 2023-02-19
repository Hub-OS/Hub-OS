use crate::resources::{ResourcePaths, IDENTITY_LEN};
use packets::address_parsing::uri_encode;
use rand::{rngs::OsRng, RngCore};

pub struct Identity {
    data: Vec<u8>,
}

impl Identity {
    pub fn for_address(address: &str) -> Self {
        let address = packets::address_parsing::strip_data(address).replace(':', "_p");
        let address = uri_encode(&address);

        let folder = ResourcePaths::IDENTITY_FOLDER.to_string();
        let file_path = folder.clone() + &address;

        let data = std::fs::read(&file_path).unwrap_or_else(|_| {
            let mut data = Vec::new();

            data.resize(IDENTITY_LEN, 0);
            OsRng.fill_bytes(&mut data);

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
