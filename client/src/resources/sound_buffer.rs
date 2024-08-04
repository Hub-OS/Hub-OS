use crate::resources::Globals;
use framework::prelude::GameIO;
use itertools::Itertools;
use rodio::Source;
use rustysynth::{MidiFile, MidiFileSequencer, Synthesizer, SynthesizerSettings};
use std::io::Cursor;
use std::sync::atomic::AtomicBool;
use std::sync::Arc;
use std::time::Duration;

#[derive(Clone)]
pub struct SoundBuffer {
    channels: u16,
    sample_rate: u32,
    duration: Duration,
    data: Arc<[i16]>,
}

impl SoundBuffer {
    pub fn decode(game_io: &GameIO, raw: Vec<u8>) -> Self {
        if raw.starts_with(b"MThd") {
            return Self::decode_midi(game_io, raw);
        }

        if raw.is_empty() {
            return Self::new_empty();
        }

        Self::decode_non_midi(raw)
    }

    pub fn decode_non_midi(raw: Vec<u8>) -> Self {
        let cursor = Cursor::new(raw);
        let Ok(decoder) = rodio::Decoder::new(cursor) else {
            return Self::new_empty();
        };

        let channels = decoder.channels();
        let sample_rate = decoder.sample_rate();
        let data: Arc<[i16]> = decoder.collect();
        let duration = data.len() as f32 / (channels as f32 * sample_rate as f32);

        Self {
            channels,
            sample_rate,
            duration: Duration::from_secs_f32(duration),
            data,
        }
    }

    fn decode_midi(game_io: &GameIO, raw: Vec<u8>) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let Some(sound_font) = globals.music.sound_font.as_ref() else {
            // no sound font loaded
            return Self::new_empty();
        };

        // create the sythesizer and midi sequencer
        let mut synth_settings = SynthesizerSettings::new(44100);
        synth_settings.enable_reverb_and_chorus = false;

        let synthesizer = Synthesizer::new(sound_font, &synth_settings).unwrap();
        let mut sequencer = MidiFileSequencer::new(synthesizer);

        // pass midi file
        let mut cursor = Cursor::new(raw);

        let Ok(midi_file) = MidiFile::new(&mut cursor) else {
            log::warn!("Invalid midi file");
            return Self::new_empty();
        };

        let midi_file = Arc::new(midi_file);
        sequencer.play(&midi_file, false);

        // the output buffer
        let sample_rate = synth_settings.sample_rate as u32;
        let sample_count = (sample_rate as f64 * midi_file.get_length()) as usize;
        let mut left: Vec<f32> = vec![0_f32; sample_count];
        let mut right: Vec<f32> = vec![0_f32; sample_count];

        sequencer.render(&mut left, &mut right);

        // merge left + right channels, alternating from left to right
        let iterator = left.into_iter().interleave(right);

        // convert into i16
        let multiplier = i16::MAX as f32;
        let data = iterator
            .map(|sample| (sample * multiplier) as i16)
            .collect();

        Self {
            channels: 2,
            sample_rate,
            duration: Duration::from_secs_f64(midi_file.get_length()),
            data,
        }
    }

    pub fn new_empty() -> Self {
        Self {
            channels: 1,
            sample_rate: 1,
            duration: Duration::ZERO,
            data: Arc::new([]),
        }
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn create_sampler(&self) -> SoundBufferSampler {
        SoundBufferSampler {
            buffer: self.clone(),
            index: 0,
            loop_range: None,
            stop_looping: Default::default(),
        }
    }

    pub fn create_looped_sampler(
        &self,
        range: Option<std::ops::Range<usize>>,
    ) -> SoundBufferSampler {
        SoundBufferSampler {
            buffer: self.clone(),
            index: 0,
            loop_range: Some(range.unwrap_or(0..self.data.len())),
            stop_looping: Default::default(),
        }
    }

    pub fn id(&self) -> usize {
        self.data.as_ptr() as usize
    }

    pub fn duration(&self) -> Duration {
        self.duration
    }
}

impl Default for SoundBuffer {
    fn default() -> Self {
        Self::new_empty()
    }
}

impl PartialEq for SoundBuffer {
    fn eq(&self, other: &Self) -> bool {
        Arc::ptr_eq(&self.data, &other.data)
    }
}

pub struct SoundBufferSampler {
    buffer: SoundBuffer,
    index: usize,
    loop_range: Option<std::ops::Range<usize>>,
    stop_looping: Arc<AtomicBool>,
}

impl SoundBufferSampler {
    pub fn end_loop_callback(&self) -> impl Fn() {
        let stop_looping = self.stop_looping.clone();

        move || {
            stop_looping.store(true, std::sync::atomic::Ordering::Relaxed);
        }
    }
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
        if self.loop_range.is_some() {
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

        if let Some(range) = self.loop_range.clone() {
            if self.index >= range.end {
                if self.stop_looping.load(std::sync::atomic::Ordering::Relaxed) {
                    self.loop_range = None;
                } else {
                    self.index = range.start;
                }
            }
        }

        sample
    }
}
