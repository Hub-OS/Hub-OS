use crate::bindable::AudioBehavior;
use crate::resources::Globals;
use crate::{render::FrameTime, resources::SoundBuffer};
use framework::common::GameIO;
use std::cell::RefCell;
use std::collections::{HashMap, HashSet};

#[derive(Default)]
struct AudioFrame {
    single_playback: HashSet<usize>,
    loops_started: HashSet<usize>,
}

#[derive(Default)]
struct PrimaryAudioCount {
    // keys are sound buffer ids, values are counts
    single_playback: HashMap<usize, usize>,
    loops_started: HashMap<usize, usize>,
}

#[derive(Default)]
struct Inner {
    primary_frame: PrimaryAudioCount,
    sound_map: HashMap<usize, SoundBuffer>,
    time_map: HashMap<FrameTime, AudioFrame>,
    rolled_back: Vec<AudioFrame>,
    prev_frames: Vec<AudioFrame>,
    open_loops: HashSet<usize>,
}

#[derive(Default)]
pub struct BattleAudioTracking {
    inner: RefCell<Inner>,
}

impl BattleAudioTracking {
    pub fn prep_resimulation(&mut self, start_time: FrameTime, end_time: FrameTime) {
        let inner = &mut *self.inner.borrow_mut();

        for i in start_time..end_time {
            if let Some(audio_frame) = inner.time_map.remove(&i) {
                inner.rolled_back.push(audio_frame);
            }
        }
    }

    pub fn end_resimulation(&mut self, game_io: &GameIO) {
        let inner = &mut *self.inner.borrow_mut();
        let primary_frame = &mut inner.primary_frame;

        let globals = Globals::from_resources(game_io);
        let audio = &globals.audio;

        for frame in inner.rolled_back.drain(..) {
            for id in &frame.loops_started {
                let Some(count) = primary_frame.loops_started.get_mut(id) else {
                    continue;
                };

                *count -= 1;

                if *count > 0 {
                    continue;
                }

                // the loop never played in the resimulation, so it'll likely never end the loop
                // end the loop now to avoid playing it for the rest of the battle
                if let Some(sound_buffer) = inner.sound_map.get(id) {
                    audio.play_sound_with_behavior(sound_buffer, AudioBehavior::EndLoop);
                }

                // no longer need to track this loop
                inner.open_loops.remove(id);
            }

            for id in &frame.single_playback {
                let Some(count) = primary_frame.loops_started.get_mut(id) else {
                    continue;
                };

                *count -= 1;
            }

            inner.prev_frames.push(frame);
        }
    }

    pub fn end_loops(&mut self, game_io: &GameIO) {
        let inner = &mut *self.inner.borrow_mut();

        let globals = Globals::from_resources(game_io);
        let audio = &globals.audio;

        for id in inner.open_loops.drain() {
            if let Some(buffer) = inner.sound_map.get(&id) {
                audio.play_sound_with_behavior(buffer, AudioBehavior::EndLoop);
            }
        }
    }

    /// Returns true if the audio can be played
    pub fn track_sound(
        &self,
        time: FrameTime,
        is_resimulation: bool,
        sound_buffer: &SoundBuffer,
        behavior: AudioBehavior,
    ) -> bool {
        let inner = &mut *self.inner.borrow_mut();

        match behavior {
            AudioBehavior::EndLoop => {
                // no longer need to track this buffer
                inner.open_loops.remove(&sound_buffer.id());
                // always end loops
                return true;
            }
            AudioBehavior::NoOverlap => return true,
            _ => {}
        }

        let time_map = &mut inner.time_map;
        let prev_frames = &mut inner.prev_frames;
        let primary_frame = &mut inner.primary_frame;

        let audio_frame = time_map
            .entry(time)
            .or_insert_with(|| prev_frames.pop().unwrap_or_default());

        fn insert_audio_id(
            primary_map: &mut HashMap<usize, usize>,
            set: &mut HashSet<usize>,
            sound_buffer: &SoundBuffer,
        ) -> Option<usize> {
            if !set.insert(sound_buffer.id()) {
                // avoid playing audio more than once in a single frame
                return None;
            }

            // increment count on the primary map
            let primary_count = primary_map.entry(sound_buffer.id()).or_default();
            *primary_count += 1;

            Some(*primary_count)
        }

        let updated_count = if matches!(behavior, AudioBehavior::LoopSection(..)) {
            insert_audio_id(
                &mut primary_frame.loops_started,
                &mut audio_frame.loops_started,
                sound_buffer,
            )
        } else {
            insert_audio_id(
                &mut primary_frame.single_playback,
                &mut audio_frame.single_playback,
                sound_buffer,
            )
        };

        let Some(new_count) = updated_count else {
            // already inserted this frame / did not update count
            // avoid playing more than once
            return false;
        };

        // only allow audio to be played if we're not resimulating, or this is the first occurrence of this sound
        let new_playback = !is_resimulation || new_count == 1;

        // remember looping buffers for rollback and exit cleanup
        if matches!(behavior, AudioBehavior::LoopSection(..)) {
            let id = sound_buffer.id();
            inner
                .sound_map
                .entry(id)
                .or_insert_with(|| sound_buffer.clone());

            inner.open_loops.insert(id);
        }

        new_playback
    }

    pub fn drop_frame(&self, time: FrameTime) {
        let inner = &mut *self.inner.borrow_mut();

        let Some(mut audio_frame) = inner.time_map.remove(&time) else {
            return;
        };

        fn decrement_set(primary_map: &mut HashMap<usize, usize>, set: &HashSet<usize>) {
            for &id in set {
                let std::collections::hash_map::Entry::Occupied(mut occupied_entry) =
                    primary_map.entry(id)
                else {
                    continue;
                };

                let count = occupied_entry.get_mut();

                *count -= 1;

                if *count == 0 {
                    occupied_entry.remove();
                }
            }
        }

        decrement_set(
            &mut inner.primary_frame.single_playback,
            &audio_frame.single_playback,
        );

        decrement_set(
            &mut inner.primary_frame.loops_started,
            &audio_frame.loops_started,
        );

        // recycle set
        audio_frame.single_playback.clear();
        audio_frame.loops_started.clear();
        inner.prev_frames.push(audio_frame);
    }
}
