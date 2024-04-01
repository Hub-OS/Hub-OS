use super::SoundBuffer;
use indexmap::IndexMap;
use rodio::cpal::{traits::HostTrait, Device};
use rodio::{DeviceTrait, OutputStream, Source};
use std::cell::RefCell;
use std::time::{Duration, Instant};

pub use crate::bindable::AudioBehavior;

pub struct AudioManager {
    stream: Option<rodio::OutputStream>,
    stream_handle: Option<rodio::OutputStreamHandle>,
    sfx_sinks: RefCell<IndexMap<usize, (Instant, rodio::Sink)>>,
    music_sink: RefCell<Option<rodio::Sink>>,
    music_stack: RefCell<Vec<(SoundBuffer, bool)>>,
    music_volume: f32,
    sfx_volume: f32,
}

impl AudioManager {
    pub fn new(name: &str) -> Self {
        let mut audio_manager = Self {
            stream: None,
            stream_handle: None,
            sfx_sinks: RefCell::new(Default::default()),
            music_sink: RefCell::new(None),
            music_stack: RefCell::new(vec![(SoundBuffer::new_empty(), false)]),
            music_volume: 1.0,
            sfx_volume: 1.0,
        };

        audio_manager.use_device(name);

        audio_manager
    }

    pub fn use_device(&mut self, name: &str) {
        let Some(device) = Self::get_device(name) else {
            log::error!("No audio output device named {name:?}");
            return;
        };

        let (stream, stream_handle) = match OutputStream::try_from_device(&device) {
            Ok((stream, stream_handle)) => (Some(stream), Some(stream_handle)),
            Err(e) => {
                log::error!("{e}");
                (None, None)
            }
        };

        self.stream = stream;
        self.stream_handle = stream_handle;
        self.restart_music();
    }

    fn get_device(name: &str) -> Option<Device> {
        if name.is_empty() {
            let host = rodio::cpal::default_host();
            return host.default_output_device();
        }

        Self::devices()
            .find(|device| matches!(device.name(), Ok(device_name) if device_name == name))
    }

    fn devices() -> impl Iterator<Item = Device> {
        let host = rodio::cpal::default_host();
        std::iter::once(host.output_devices()).flatten().flatten()
    }

    pub fn device_names() -> impl Iterator<Item = String> {
        Self::devices().flat_map(|device| device.name())
    }

    pub fn with_music_volume(mut self, volume: f32) -> Self {
        self.set_music_volume(volume);
        self
    }

    pub fn with_sfx_volume(mut self, volume: f32) -> Self {
        self.set_sfx_volume(volume);
        self
    }

    pub fn set_music_volume(&mut self, volume: f32) {
        self.music_volume = volume;

        if let Some(music_sink) = self.music_sink.get_mut() {
            music_sink.set_volume(volume);
        }
    }

    pub fn set_sfx_volume(&mut self, volume: f32) {
        self.sfx_volume = volume;
    }

    pub fn is_music_playing(&self) -> bool {
        matches!(&*self.music_sink.borrow(), Some(sink) if !sink.empty())
    }

    pub fn current_music(&self) -> Option<SoundBuffer> {
        let stack = self.music_stack.borrow();
        stack.last().map(|(buffer, _)| buffer).cloned()
    }

    pub fn music_stack_len(&self) -> usize {
        self.music_stack.borrow().len()
    }

    pub fn truncate_music_stack(&self, size: usize) -> bool {
        let mut stack = self.music_stack.borrow_mut();

        if stack.len() == size {
            return false;
        }

        stack.truncate(size);
        std::mem::drop(stack);
        self.stop_music();

        true
    }

    pub fn restart_music(&self) {
        let stack = self.music_stack.borrow();
        let (buffer, loops) = stack.last().cloned().unwrap();

        std::mem::drop(stack);
        self.play_music(&buffer, loops);
    }

    pub fn push_music_stack(&self) {
        self.stop_music();
        let mut stack = self.music_stack.borrow_mut();
        stack.push((SoundBuffer::new_empty(), false));
    }

    pub fn pop_music_stack(&self) {
        self.stop_music();
        let mut stack = self.music_stack.borrow_mut();
        stack.pop();
    }

    pub fn play_music(&self, buffer: &SoundBuffer, loops: bool) {
        let Some(stream_handle) = self.stream_handle.as_ref() else {
            return;
        };

        if let Some(music_sink) = self.music_sink.borrow().as_ref() {
            music_sink.stop();
        }

        if buffer.is_empty() {
            // empty buffer, just return after stopping music
            // fixes unrecognized format error
            return;
        }

        let music_sink = match rodio::Sink::try_new(stream_handle) {
            Ok(music_sink) => music_sink,
            Err(e) => {
                log::error!("Failed to create music sink: {e}");
                return;
            }
        };

        music_sink.set_volume(self.music_volume);

        if loops {
            music_sink.append(buffer.create_looped_sampler());
        } else {
            music_sink.append(buffer.create_sampler());
        }

        *self.music_sink.borrow_mut() = Some(music_sink);
        *(self.music_stack.borrow_mut().last_mut().unwrap()) = (buffer.clone(), loops);
    }

    pub fn stop_music(&self) {
        if let Some(music_sink) = self.music_sink.borrow().as_ref() {
            music_sink.stop();
        }
    }

    pub fn play_sound(&self, buffer: &SoundBuffer) {
        let Some(stream_handle) = self.stream_handle.as_ref() else {
            return;
        };

        let source = buffer
            .create_sampler()
            .convert_samples()
            .amplify(self.sfx_volume);

        let res = stream_handle.play_raw(source);

        if let Err(e) = res {
            log::error!("{e}");
        }
    }

    pub fn play_sound_with_behavior(&self, buffer: &SoundBuffer, behavior: AudioBehavior) {
        match behavior {
            AudioBehavior::Default => {
                self.play_sound(buffer);
                return;
            }
            AudioBehavior::NoOverlap => {}
        }

        let Some(stream_handle) = self.stream_handle.as_ref() else {
            return;
        };

        let mut sfx_sinks = self.sfx_sinks.borrow_mut();
        let (start_instant, sfx_sink);

        if let Some((instant, sink)) = sfx_sinks.get_mut(&buffer.id()) {
            // reuse sink
            start_instant = instant;
            sfx_sink = sink;
        } else {
            // create a new sink
            let sink = match rodio::Sink::try_new(stream_handle) {
                Ok(sfx_sink) => sfx_sink,
                Err(e) => {
                    log::error!("Failed to create sfx sink: {e}");
                    return;
                }
            };

            sfx_sinks.insert(buffer.id(), (Instant::now(), sink));

            // get the newly created sink
            let (instant, sink) = sfx_sinks.get_mut(&buffer.id()).unwrap();
            start_instant = instant;
            sfx_sink = sink;
        }

        // queue if there's no sfx queued, or the currently queued sfx is about to end
        let can_queue = sfx_sink.empty()
            || (sfx_sink.len() == 1
                && buffer.duration().saturating_sub(start_instant.elapsed())
                    < Duration::from_millis(17));

        if can_queue {
            let source = buffer
                .create_sampler()
                .convert_samples::<f32>()
                .amplify(self.sfx_volume);

            sfx_sink.append(source);
            *start_instant = Instant::now();
        }
    }

    pub fn drop_empty_sinks(&self) {
        let mut sfx_sinks = self.sfx_sinks.borrow_mut();

        sfx_sinks.retain(|_, (_, sink)| !sink.empty());
    }
}
