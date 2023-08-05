use crate::resources::Globals;
use framework::prelude::{GameIO, GameService};

pub struct SupportingService {
    suspended_music: bool,
}

impl SupportingService {
    pub fn new() -> Self {
        Self {
            suspended_music: false,
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
    }

    fn post_update(&mut self, game_io: &mut GameIO) {
        let suspended = game_io.suspended();
        let globals = game_io.resource::<Globals>().unwrap();

        if suspended {
            // stop music as another thread controls audio and would continue playing in the background
            self.suspended_music = globals.audio.is_music_playing();

            if self.suspended_music {
                globals.audio.stop_music();
            }
        }

        globals.audio.drop_empty_sinks();
    }
}
