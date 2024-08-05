use super::State;
use crate::battle::{BattleSimulation, SharedBattleResources};
use crate::render::ui::BattleBannerMessage;
use crate::render::{FrameTime, SpriteColorQueue};
use framework::prelude::GameIO;
use std::borrow::Cow;

const DISPLAY_DURATION: FrameTime = 120;

#[derive(Clone)]
pub struct TimeUpState {
    banner: BattleBannerMessage,
}

impl TimeUpState {
    pub fn new() -> Self {
        let mut banner = BattleBannerMessage::new(Cow::Borrowed("<_TIME_UP!_>"));
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
            simulation.exit = true;
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
