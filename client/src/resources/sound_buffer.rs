use crate::resources::Globals;
use framework::prelude::GameIO;
use itertools::Itertools;
use rodio::Source;
use rustysynth::{MidiFile, MidiFileSequencer, Synthesizer, SynthesizerSettings};
use std::io::Cursor;
use std::sync::Arc;
use std::sync::atomic::AtomicBool;
use std::time::Duration;

fn meta_value_as_usize(value: &symphonia::core::meta::Value) -> usize {
    use symphonia::core::meta::Value;

    match value {
        Value::Binary(_) => 0,
        Value::Boolean(_) => 0,
        Value::Flag => 0,
        Value::Float(n) => *n as _,
        Value::SignedInt(n) => *n as _,
        Value::String(s) => s.parse().unwrap_or_default(),
        Value::UnsignedInt(n) => *n as _,
    }
}

#[derive(Clone)]
pub struct SoundBuffer {
    pub default_loop_range: std::ops::Range<usize>,
    pub channels: u16,
    pub sample_rate: u32,
    pub duration: Duration,
    pub data: Arc<[i16]>,
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
        // https://docs.rs/symphonia/latest/symphonia/index.html
        use symphonia::core::audio::SampleBuffer;
        use symphonia::core::io::MediaSourceStream;

        let cursor = Cursor::new(raw);

        // 1 + 2
        let probe = symphonia::default::get_probe();
        let codecs = symphonia::default::get_codecs();

        // "4. Instantiate a MediaSourceStream with the MediaSource above."
        let stream = MediaSourceStream::new(Box::new(cursor), Default::default());

        // "5. Using the Probe, call format and pass it the MediaSourceStream."
        let Ok(probe_result) = probe.format(
            &Default::default(),
            stream,
            &Default::default(),
            &Default::default(),
        ) else {
            return Self::new_empty();
        };

        // "7. At this point it is possible to interrogate the FormatReader for general information about the media and metadata.
        //     Examine the Track listing using tracks and select one or more tracks of interest to decode."
        let mut format = probe_result.format;
        let Some(track) = format.default_track() else {
            return Self::new_empty();
        };

        let track_id = track.id;
        let channels = track.codec_params.channels.unwrap_or_default().count();

        if channels == 0 {
            // avoiding dividing by zero
            // also if there's no channels, there's no audio
            return Self::new_empty();
        }

        let sample_rate = track.codec_params.sample_rate.unwrap_or(44100); // 44.1 kHz

        // "8. To instantiate a Decoder for a selected Track, call the CodecRegistry’s make function and pass it the CodecParameters for that track.
        //     This step is repeated once per selected track."
        let Ok(mut decoder) = codecs.make(&track.codec_params, &Default::default()) else {
            return Self::new_empty();
        };

        // "9. To decode a track, obtain a packet from the FormatReader by calling next_packet and then pass the Packet to the Decoder for that track.
        //     The decode function will read a packet and return an AudioBufferRef (an “any-type” AudioBuffer)."
        let mut data = Vec::new();
        let mut sample_buf = None;
        let mut loop_start = 0;
        let mut loop_end = None;
        let mut loop_length = None;

        // process metadata
        if let Some(metadata_revision) = format.metadata().current() {
            for tag in metadata_revision.tags() {
                match tag.key.as_str() {
                    "LOOP_START" | "LOOPSTART" => {
                        loop_start = meta_value_as_usize(&tag.value) * channels
                    }
                    "LOOP_END" | "LOOPEND" => {
                        loop_end = Some(meta_value_as_usize(&tag.value) * channels)
                    }
                    "LOOP_LENGTH" | "LOOPLENGTH" | "LOOP_LEN" | "LOOPLEN" => {
                        loop_length = Some(meta_value_as_usize(&tag.value) * channels)
                    }
                    _ => {}
                }
            }
        }

        // read data
        while let Ok(packet) = format.next_packet() {
            if packet.track_id() != track_id {
                // skip data for other tracks
                continue;
            }

            let Ok(audio_buf) = decoder.decode(&packet) else {
                continue;
            };

            // https://github.com/pdeljanov/Symphonia/blob/master/symphonia/examples/basic-interleaved.rs
            if sample_buf.is_none() {
                // Get the audio buffer specification.
                let spec = *audio_buf.spec();

                // Get the capacity of the decoded buffer. Note: This is capacity, not length!
                let duration = audio_buf.capacity() as u64;

                // Create the sample buffer.
                sample_buf = Some(SampleBuffer::<i16>::new(duration, spec));
            }

            // Copy the decoded audio buffer into the sample buffer in an interleaved format.
            if let Some(buf) = &mut sample_buf {
                buf.copy_interleaved_ref(audio_buf);

                data.extend(buf.samples());
            }
        }

        let data: Arc<[i16]> = data.into();
        let duration = data.len() as f32 / (channels as f32 * sample_rate as f32);

        // resolve loop range
        loop_start = loop_start.min(data.len());
        let mut loop_range = loop_start..data.len();

        if let Some(len) = loop_length {
            loop_range.end = loop_range.end.min(loop_start + len);
        }

        if let Some(end) = loop_end {
            loop_range.end = loop_range.end.min(end).max(loop_start);
        }

        Self {
            default_loop_range: loop_range,
            channels: channels as _,
            sample_rate,
            duration: Duration::from_secs_f32(duration),
            data,
        }
    }

    fn decode_midi(game_io: &GameIO, raw: Vec<u8>) -> Self {
        let globals = Globals::from_resources(game_io);
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
        let data: Arc<[_]> = iterator
            .map(|sample| (sample * multiplier) as i16)
            .collect();

        Self {
            default_loop_range: 0..data.len(),
            channels: 2,
            sample_rate,
            duration: Duration::from_secs_f64(midi_file.get_length()),
            data,
        }
    }

    pub fn new_empty() -> Self {
        Self {
            default_loop_range: 0..0,
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
            loop_range: Some(range.unwrap_or(self.default_loop_range.clone())),
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
    pub fn end_loop_callback(&self) -> impl Fn() + use<> {
        let stop_looping = self.stop_looping.clone();

        move || {
            stop_looping.store(true, std::sync::atomic::Ordering::Relaxed);
        }
    }
}

impl Source for SoundBufferSampler {
    fn current_span_len(&self) -> Option<usize> {
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
    type Item = f32;

    fn next(&mut self) -> Option<Self::Item> {
        let sample = self.buffer.data.get(self.index).cloned();
        self.index += 1;

        if let Some(range) = self.loop_range.clone()
            && self.index >= range.end
        {
            if self.stop_looping.load(std::sync::atomic::Ordering::Relaxed) {
                self.loop_range = None;
            } else {
                self.index = range.start;
            }
        }

        let sample = sample?;
        let sample = dasp_sample::Sample::from_sample(sample);

        Some(sample)
    }
}
