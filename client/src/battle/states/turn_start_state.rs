use super::{BattleState, State};
use crate::battle::{BattleSimulation, SharedBattleResources};
use crate::bindable::GenerationalIndex;
use crate::render::ui::{BattleBannerMessage, BattleBannerPopup};
use crate::render::{FrameTime, SpriteColorQueue};
use framework::prelude::GameIO;

const DELAY: FrameTime = 45;
const DISPLAY_DURATION: FrameTime = 60;

#[derive(Clone)]
pub struct TurnStartState {
    time: FrameTime,
    banner_key: Option<GenerationalIndex>,
    complete: bool,
}

impl TurnStartState {
    pub fn new() -> Self {
        Self {
            time: 0,
            banner_key: None,
            complete: false,
        }
    }

    fn spawn_banner(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let turn_number = simulation.statistics.turns;
        let config = resources.config.borrow();
        let message = if config.turn_limit == Some(turn_number) {
            BattleBannerMessage::FinalTurn
        } else if config.turn_limit.is_some() {
            BattleBannerMessage::TurnNumberStart(turn_number)
        } else {
            BattleBannerMessage::TurnStart
        };

        let mut banner = BattleBannerPopup::new(game_io, message);
        banner.show_for(DISPLAY_DURATION);

        let key = simulation.banner_popups.insert(banner);
        self.banner_key = Some(key);
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
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        if simulation.inputs.iter().any(|input| input.fleeing()) {
            // wait for the flee attempt to complete
            return;
        }

        if self.time == DELAY {
            self.spawn_banner(game_io, resources, simulation);
        } else if let Some(key) = self.banner_key {
            self.complete = !simulation.banner_popups.contains_key(key);
        }

        self.time += 1;
    }

    fn draw_ui<'a>(
        &mut self,
        _game_io: &'a GameIO,
        _resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        // turn gauge
        if simulation.hud_visible() {
            simulation.turn_gauge.draw(sprite_queue);
        }
    }
}
