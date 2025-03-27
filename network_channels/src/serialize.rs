pub use bincode::error::DecodeError;

use bincode::config::Configuration as BincodeConfig;
const BINCODE_CONFIG: BincodeConfig = bincode::config::standard();

pub fn serialize(value: impl serde::Serialize) -> Vec<u8> {
    bincode::serde::encode_to_vec(value, BINCODE_CONFIG).unwrap()
}

pub fn serialize_into<W: bincode::enc::write::Writer>(buf: &mut W, value: impl serde::Serialize) {
    bincode::serde::encode_into_writer(value, buf, BINCODE_CONFIG).unwrap();
}

pub fn deserialize<'de, V: serde::Deserialize<'de>>(bytes: &'de [u8]) -> Result<V, DecodeError> {
    bincode::serde::borrow_decode_from_slice(bytes, BINCODE_CONFIG).map(|(v, _)| v)
}
