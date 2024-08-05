use super::{BattleState, State};
use crate::battle::{BattleSimulation, SharedBattleResources};
use crate::render::ui::BattleBannerMessage;
use crate::render::{FrameTime, SpriteColorQueue};
use framework::prelude::GameIO;
use std::borrow::Cow;

const DELAY: FrameTime = 45;
const DISPLAY_DURATION: FrameTime = 60;

#[derive(Clone)]
pub struct TurnStartState {
    banner: BattleBannerMessage,
    time: FrameTime,
    complete: bool,
}

impl TurnStartState {
    pub fn new() -> Self {
        Self {
            banner: BattleBannerMessage::default(),
            complete: false,
            time: 0,
        }
    }

    fn init_message(&mut self, simulation: &BattleSimulation) {
        let turn_number = simulation.statistics.turns;
        let message = if simulation.config.turn_limit == Some(turn_number) {
            Cow::Borrowed("<_FINAL_TURN!_>")
        } else if simulation.config.turn_limit.is_some() {
            Cow::Owned(format!("<_TURN_{}_START!_>", turn_number))
        } else {
            Cow::Borrowed("<_TURN_START!_>")
        };

        self.banner.set_message(message);
    }
}

impl State for TurnStartState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, _: &GameIO) -> Option<Box<dyn State>> {
        if self.complete {
            Some(Box::new(BattleState::new()))
        } else {
            None
        }
    }

    fn update(
        &mut self,
        _game_io: &GameIO,
        _resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        if simulation.inputs.iter().any(|input| input.fleeing()) {
            // wait for the flee attempt to complete
            return;
        }

        if self.time == 0 {
            self.init_message(simulation);
        } else if self.time == DELAY {
            self.banner.show_for(DISPLAY_DURATION);
        } else if let Some(t) = self.banner.remaining_time() {
            self.complete = t == 0;
            self.banner.update();
        }

        self.time += 1;
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO,
        _resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        // turn gauge
        simulation.turn_gauge.draw(sprite_queue);
        self.banner.draw(game_io, sprite_queue);
    }
}
