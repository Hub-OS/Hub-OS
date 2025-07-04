use crate::packages::PackageNamespace;
use crate::resources::{Globals, RESOLUTION};
use crate::saves::InternalResolution;
use framework::input::Key;
use framework::prelude::{GameIO, GameService};
use packets::structures::PackageCategory;

pub enum SupportingServiceEvent {
    LoadPackage {
        category: PackageCategory,
        namespace: PackageNamespace,
        path: String,
    },
}

#[derive(Clone)]
pub struct SupportingServiceComm {
    sender: flume::Sender<SupportingServiceEvent>,
}

impl SupportingServiceComm {
    pub fn send(&self, event: SupportingServiceEvent) {
        self.sender.send(event).unwrap();
    }
}

pub struct SupportingService {
    suspended_music: bool,
    receiver: flume::Receiver<SupportingServiceEvent>,
}

impl SupportingService {
    pub fn new(game_io: &mut GameIO) -> Self {
        let (sender, receiver) = flume::unbounded();

        game_io.set_resource(SupportingServiceComm { sender });

        Self {
            suspended_music: false,
            receiver,
        }
    }
}

impl GameService for SupportingService {
    fn pre_update(&mut self, game_io: &mut GameIO) {
        let suspended = game_io.suspended();
        let globals = game_io.resource_mut::<Globals>().unwrap();

        if !suspended && self.suspended_music {
            // resume music if we stopped it
            globals.audio.restart_music();
            self.suspended_music = false;
        }

        // handle internal resolution and snap resize
        let internal_resolution = globals.internal_resolution;
        let snap_resize = globals.snap_resize;
        let window = game_io.window_mut();

        if window.has_locked_resolution() {
            // lock back to a base resolution to resolve the render scale
            // lock_resolution only updates a few variables so this is fast
            let base_resolution = match internal_resolution {
                InternalResolution::Default => RESOLUTION * 2,
                _ => RESOLUTION,
            };
            window.lock_resolution(base_resolution);

            let target_size = base_resolution.as_vec2() * window.render_scale();
            let target_size = target_size.as_uvec2();

            let updated_resolution = match internal_resolution {
                InternalResolution::Auto => target_size,
                _ => base_resolution,
            };

            if snap_resize {
                if target_size != window.size() {
                    window.request_size(target_size);
                    window.lock_resolution(updated_resolution);
                }
            } else {
                window.lock_resolution(updated_resolution);
            }
        }

        // save recording
        if game_io.input().is_key_down(Key::F3) && game_io.input().was_key_just_pressed(Key::S) {
            let globals = game_io.resource_mut::<Globals>().unwrap();

            if let Some((props, recording)) = globals.battle_recording.take() {
                recording.save(game_io, &props);
            } else {
                log::error!("No recording. Try again after a battle or check settings.");
            }
        }
    }

    fn post_update(&mut self, game_io: &mut GameIO) {
        let suspended = game_io.suspended();
        let globals = game_io.resource_mut::<Globals>().unwrap();

        if suspended {
            // stop music as another thread controls audio and would continue playing in the background
            self.suspended_music = globals.audio.is_music_playing();

            if self.suspended_music {
                globals.audio.stop_music();
            }
        }

        globals.audio.drop_empty_sinks();

        while let Ok(event) = self.receiver.try_recv() {
            match event {
                SupportingServiceEvent::LoadPackage {
                    category,
                    namespace,
                    path,
                } => {
                    globals.load_package(category, namespace, &path);
                }
            }
        }
    }
}
