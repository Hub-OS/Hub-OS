use crate::packages::PackageNamespace;
use crate::resources::{Globals, RESOLUTION};
use crate::saves::InternalResolution;
use framework::input::Key;
use framework::prelude::{GameIO, GameService};
use packets::structures::PackageCategory;

pub enum SupportingServiceEvent {
    Saving,
    SavingEnd,
    Quit,
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
        let _ = self.sender.send(event);
    }
}

pub struct SupportingService {
    pending_save_count: usize,
    receiver: flume::Receiver<SupportingServiceEvent>,
}

impl SupportingService {
    pub fn new(game_io: &mut GameIO) -> Self {
        let (sender, receiver) = flume::unbounded();

        game_io.set_resource(SupportingServiceComm { sender });

        Self {
            pending_save_count: 0,
            receiver,
        }
    }

    #[cfg(not(target_os = "android"))]
    fn handle_quit(&mut self, game_io: &mut GameIO) {
        if self.pending_save_count == 0 || !game_io.quitting() {
            return;
        }

        game_io.cancel_quit();

        let Some(comm) = game_io.resource::<SupportingServiceComm>() else {
            return;
        };

        let comm = comm.clone();
        let globals = Globals::from_resources(game_io);

        use native_dialog::{DialogBuilder, MessageLevel};

        let dialog = DialogBuilder::message()
            .set_owner(&Box::new(game_io.window()))
            .set_level(MessageLevel::Warning)
            .set_title(globals.translate("navigation-quit-without-saving-title"))
            .set_text(globals.translate("navigation-quit-while-saving-question"))
            .confirm();

        game_io
            .spawn_local_task(async move {
                let result = dialog.spawn().await;

                let cancelled = result.is_ok_and(|accepted| !accepted);

                if !cancelled {
                    // quit as long as the user didn't explictly cancel
                    comm.send(SupportingServiceEvent::Quit);
                }
            })
            .detach();
    }
}

impl GameService for SupportingService {
    fn pre_update(&mut self, game_io: &mut GameIO) {
        let globals = Globals::from_resources_mut(game_io);

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
            let globals = Globals::from_resources_mut(game_io);

            if let Some((props, recording, preview)) = globals.battle_recording.take() {
                recording.save(game_io, &props, preview);
            } else {
                log::error!("No recording. Try again after a battle or check settings.");
            }
        }
    }

    fn post_update(&mut self, game_io: &mut GameIO) {
        #[cfg(not(target_os = "android"))]
        self.handle_quit(game_io);

        let suspended = game_io.suspended();
        let globals = Globals::from_resources_mut(game_io);

        globals.audio.set_suspended(suspended);
        globals.audio.drop_empty_sinks();

        while let Ok(event) = self.receiver.try_recv() {
            match event {
                SupportingServiceEvent::Saving => {
                    self.pending_save_count += 1;
                }
                SupportingServiceEvent::SavingEnd => {
                    self.pending_save_count -= 1;
                }
                SupportingServiceEvent::Quit => {
                    game_io.quit();
                }
                SupportingServiceEvent::LoadPackage {
                    category,
                    namespace,
                    path,
                } => {
                    let globals = Globals::from_resources_mut(game_io);
                    globals.load_package(category, namespace, &path);
                }
            }
        }
    }
}
