use super::State;
use crate::battle::{BattleProgress, BattleSimulation, SharedBattleResources};
use crate::render::ui::{BattleBannerMessage, BattleBannerPopup};
use crate::render::{FrameTime, SpriteColorQueue};
use framework::prelude::GameIO;

const DISPLAY_DURATION: FrameTime = 120;

#[derive(Clone)]
pub struct TimeUpState {
    banner: BattleBannerPopup,
}

impl TimeUpState {
    pub fn new(game_io: &GameIO) -> Self {
        let mut banner = BattleBannerPopup::new(game_io, BattleBannerMessage::TimeUp);
        banner.show_for(DISPLAY_DURATION);

        Self { banner }
    }
}

impl State for TimeUpState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, _: &GameIO) -> Option<Box<dyn State>> {
        None
    }

    fn update(
        &mut self,
        _game_io: &GameIO,
        _resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        self.banner.update();

        if self.banner.remaining_time().is_some_and(|t| t == 0) {
            simulation.progress = BattleProgress::Exiting;
        }
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO,
        _resources: &SharedBattleResources,
        _simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        self.banner.draw(game_io, sprite_queue);
    }
}
