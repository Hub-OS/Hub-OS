use rodio::Source;
use std::sync::Arc;
use std::time::Duration;

#[derive(Clone)]
pub struct SoundBuffer {
    channels: u16,
    sample_rate: u32,
    duration: Duration,
    data: Arc<Vec<i16>>,
}

impl SoundBuffer {
    pub fn decode(raw: Vec<u8>) -> Self {
        use std::io::Cursor;

        let cursor = Cursor::new(raw);
        let Ok(decoder) = rodio::Decoder::new(cursor) else {
            return Self::new_empty();
        };

        Self {
            channels: decoder.channels(),
            sample_rate: decoder.sample_rate(),
            duration: decoder.total_duration().unwrap_or_default(),
            data: Arc::new(decoder.collect::<Vec<_>>()),
        }
    }

    pub fn new_empty() -> Self {
        Self {
            channels: 1,
            sample_rate: 1,
            duration: Duration::ZERO,
            data: Arc::new(Vec::new()),
        }
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn create_sampler(&self) -> SoundBufferSampler {
        SoundBufferSampler {
            looped: false,
            index: 0,
            buffer: self.clone(),
        }
    }

    pub fn create_looped_sampler(&self) -> SoundBufferSampler {
        SoundBufferSampler {
            looped: true,
            index: 0,
            buffer: self.clone(),
        }
    }
}

impl Default for SoundBuffer {
    fn default() -> Self {
        Self::new_empty()
    }
}

pub struct SoundBufferSampler {
    looped: bool,
    index: usize,
    buffer: SoundBuffer,
}

impl Source for SoundBufferSampler {
    fn current_frame_len(&self) -> Option<usize> {
        None
    }

    fn channels(&self) -> u16 {
        self.buffer.channels
    }

    fn sample_rate(&self) -> u32 {
        self.buffer.sample_rate
    }

    fn total_duration(&self) -> Option<Duration> {
        if self.looped {
            None
        } else {
            Some(self.buffer.duration)
        }
    }
}

impl Iterator for SoundBufferSampler {
    type Item = i16;

    fn next(&mut self) -> Option<Self::Item> {
        let sample = self.buffer.data.get(self.index).cloned();
        self.index += 1;

        if self.looped {
            let buffer_len = self.buffer.data.len();
            self.index = self.index.checked_rem(buffer_len).unwrap_or_default();
        }

        sample
    }
}
