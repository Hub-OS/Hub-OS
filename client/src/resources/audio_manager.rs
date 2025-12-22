use super::SoundBuffer;
use indexmap::IndexMap;
use rodio::cpal::{Device, traits::HostTrait};
use rodio::{DeviceTrait, OutputStream, OutputStreamBuilder, Source};
use std::cell::RefCell;
use std::time::{Duration, Instant};

pub use crate::bindable::AudioBehavior;

pub struct AudioManager {
    stream: Option<OutputStream>,
    sfx_sinks: RefCell<IndexMap<usize, (Instant, rodio::Sink, Option<Box<dyn Fn()>>)>>,
    music_sink: RefCell<Option<rodio::Sink>>,
    music_stack: RefCell<Vec<(SoundBuffer, bool)>>,
    music_volume: f32,
    sfx_volume: f32,
    suspended: bool,
}

impl AudioManager {
    pub fn new(name: &str) -> Self {
        let mut audio_manager = Self {
            stream: None,
            sfx_sinks: RefCell::new(Default::default()),
            music_sink: RefCell::new(None),
            music_stack: RefCell::new(vec![(SoundBuffer::new_empty(), false)]),
            music_volume: 1.0,
            sfx_volume: 1.0,
            suspended: false,
        };

        audio_manager.use_device(name);

        audio_manager
    }

    pub fn use_device(&mut self, name: &str) {
        let Some(device) = Self::get_device(name) else {
            log::error!("No audio output device named {name:?}");
            return;
        };

        self.stream = OutputStreamBuilder::from_device(device)
            .and_then(|builder| builder.open_stream())
            .map(|mut stream| {
                stream.log_on_drop(false);
                stream
            })
            .inspect_err(|err| log::error!("{err}"))
            .ok();

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

    pub fn set_suspended(&mut self, suspended: bool) {
        if self.suspended == suspended {
            return;
        }
        self.suspended = suspended;

        if let Some(music_sink) = self.music_sink.get_mut() {
            if suspended {
                music_sink.set_volume(0.0);
            } else {
                music_sink.set_volume(self.music_volume);
            }
        }
    }

    pub fn restart_music(&self) {
        if self.suspended {
            return;
        }

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
        let Some(stream) = self.stream.as_ref() else {
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

        // create a new sink
        let music_sink = rodio::Sink::connect_new(stream.mixer());

        if !self.suspended {
            music_sink.set_volume(self.music_volume);

            if loops {
                music_sink.append(buffer.create_looped_sampler(None));
            } else {
                music_sink.append(buffer.create_sampler());
            }
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
        if self.suspended {
            return;
        }

        let Some(stream) = self.stream.as_ref() else {
            return;
        };

        let source = buffer.create_sampler().amplify(self.sfx_volume);

        stream.mixer().add(source);
    }

    pub fn play_sound_with_behavior(&self, buffer: &SoundBuffer, behavior: AudioBehavior) {
        match behavior {
            AudioBehavior::Default => self.play_sound(buffer),
            AudioBehavior::Overlap => self.play_sound(buffer),
            AudioBehavior::NoOverlap => self.play_no_overlap(buffer),
            AudioBehavior::Restart => self.play_restart(buffer),
            AudioBehavior::LoopSection(start, end) => self.play_loop_section(buffer, start, end),
            AudioBehavior::EndLoop => self.end_sound_loop(buffer),
        }
    }

    fn try_ensure_sink(&self, buffer: &SoundBuffer) {
        let Some(stream) = self.stream.as_ref() else {
            return;
        };

        let mut sfx_sinks = self.sfx_sinks.borrow_mut();

        if sfx_sinks.contains_key(&buffer.id()) {
            return;
        }

        // create a new sink
        let sink = rodio::Sink::connect_new(stream.mixer());

        sfx_sinks.insert(buffer.id(), (Instant::now(), sink, None));
    }

    fn play_restart(&self, buffer: &SoundBuffer) {
        if self.suspended {
            return;
        }

        self.try_ensure_sink(buffer);

        let mut sfx_sinks = self.sfx_sinks.borrow_mut();

        let Some((start_instant, sfx_sink, end_loop)) = sfx_sinks.get_mut(&buffer.id()) else {
            return;
        };

        if end_loop.is_some() {
            // audio is looping, avoid ending the loop
            return;
        }

        let source = buffer.create_sampler().amplify(self.sfx_volume);

        sfx_sink.stop();
        sfx_sink.append(source);
        *start_instant = Instant::now();
    }

    fn play_no_overlap(&self, buffer: &SoundBuffer) {
        if self.suspended {
            return;
        }

        self.try_ensure_sink(buffer);

        let mut sfx_sinks = self.sfx_sinks.borrow_mut();

        let Some((start_instant, sfx_sink, end_loop)) = sfx_sinks.get_mut(&buffer.id()) else {
            return;
        };

        if end_loop.is_some() {
            // blocked by looping audio
            return;
        }

        // queue if there's no sfx queued, or the currently queued sfx is about to end
        let can_queue = sfx_sink.empty()
            || (sfx_sink.len() == 1
                && buffer.duration().saturating_sub(start_instant.elapsed())
                    < Duration::from_millis(17));

        if can_queue {
            let source = buffer.create_sampler().amplify(self.sfx_volume);

            sfx_sink.append(source);
            *start_instant = Instant::now();
        }
    }

    fn play_loop_section(&self, buffer: &SoundBuffer, start: usize, end: usize) {
        if self.suspended {
            return;
        }

        let Some(stream) = self.stream.as_ref() else {
            return;
        };

        if start > end {
            return;
        }

        let mut sfx_sinks = self.sfx_sinks.borrow_mut();

        // create a new sink
        let sink = rodio::Sink::connect_new(stream.mixer());

        // convert sample index for a single chanel to buffer index
        let channels = buffer.channels as usize;
        let range = (start * channels)..(end * channels);

        let sampler = buffer.create_looped_sampler(Some(range));
        let callback = Box::new(sampler.end_loop_callback());

        let source = sampler.amplify(self.sfx_volume);

        sink.append(source);
        sink.play();

        sfx_sinks.insert(buffer.id(), (Instant::now(), sink, Some(callback)));
    }

    fn end_sound_loop(&self, buffer: &SoundBuffer) {
        let mut sfx_sinks = self.sfx_sinks.borrow_mut();

        let Some((_, _, end_loop)) = sfx_sinks.get_mut(&buffer.id()) else {
            return;
        };

        if let Some(callback) = end_loop.as_ref() {
            callback();
            *end_loop = None;
        }
    }

    pub fn drop_empty_sinks(&self) {
        let mut sfx_sinks = self.sfx_sinks.borrow_mut();

        sfx_sinks.retain(|_, (_, sink, _)| !sink.empty());
    }
}
