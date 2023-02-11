use std::sync::Arc;

#[derive(Clone)]
pub struct SoundBuffer(pub Arc<Vec<u8>>);

impl SoundBuffer {
    pub fn new_empty() -> Self {
        Self(Arc::new(Vec::new()))
    }

    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
}

impl AsRef<[u8]> for SoundBuffer {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}
