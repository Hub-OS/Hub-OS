use crate::resources::Globals;
use crate::{packages::PackageNamespace, TRUE_RESOLUTION};
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
        globals.network.tick();

        if !suspended && self.suspended_music {
            // resume music if we stopped it
            globals.audio.restart_music();
            self.suspended_music = false;
        }

        // handle snap resize
        let snap_resize = globals.snap_resize;
        let window = game_io.window_mut();

        if snap_resize && window.has_locked_resolution() {
            let target_size = TRUE_RESOLUTION.as_vec2() * window.render_scale();
            let target_size = target_size.as_uvec2();

            if target_size != window.size() {
                window.request_size(target_size);
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
