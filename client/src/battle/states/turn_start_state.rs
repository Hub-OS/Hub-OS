use super::{BattleState, State};
use crate::battle::{BattleSimulation, RollbackVM, SharedBattleAssets};
use crate::ease::inverse_lerp;
use crate::render::ui::{FontStyle, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::RESOLUTION_F;
use framework::prelude::GameIO;

const DELAY: FrameTime = 45;
const REVEAL_DURATION: FrameTime = 7;
const BOUNCE_BACK_DURATION: FrameTime = 3;
const IDLE_DURATION: FrameTime = 40;
const MAX_DURATION: FrameTime =
    DELAY + (REVEAL_DURATION + BOUNCE_BACK_DURATION) * 2 + IDLE_DURATION;

#[derive(Clone)]
pub struct TurnStartState {
    message: String,
    start_time: FrameTime,
    complete: bool,
}

impl TurnStartState {
    pub fn new() -> Self {
        Self {
            complete: false,
            message: String::new(),
            start_time: 0,
        }
    }

    fn init_message(&mut self, simulation: &BattleSimulation) {
        let turn_number = simulation.statistics.turns;
        self.message = if simulation.config.turn_limit == Some(turn_number) {
            String::from("<_FINAL_TURN!_>")
        } else if simulation.config.turn_limit.is_some() {
            format!("<_TURN_{}_START!_>", turn_number)
        } else {
            String::from("<_TURN_START!_>")
        };
    }

    fn resolve_message_scale(&self, simulation: &BattleSimulation) -> f32 {
        fn delay_scale(_: FrameTime) -> f32 {
            0.0
        }

        fn reveal_scale(elapsed_time: FrameTime) -> f32 {
            inverse_lerp!(0, REVEAL_DURATION, elapsed_time) * 1.2
        }

        fn reveal_inverse_scale(elapsed_time: FrameTime) -> f32 {
            inverse_lerp!(REVEAL_DURATION, 0, elapsed_time) * 1.2
        }

        fn bounce_back_scale(elapsed_time: FrameTime) -> f32 {
            1.0 + inverse_lerp!(BOUNCE_BACK_DURATION, 0, elapsed_time) * 0.2
        }

        fn bounce_back_inverse_scale(elapsed_time: FrameTime) -> f32 {
            1.0 + inverse_lerp!(0, BOUNCE_BACK_DURATION, elapsed_time) * 0.2
        }

        fn idle_scale(_: FrameTime) -> f32 {
            1.0
        }

        let steps: [(FrameTime, fn(FrameTime) -> f32); 6] = [
            (DELAY, delay_scale),
            (REVEAL_DURATION, reveal_scale),
            (BOUNCE_BACK_DURATION, bounce_back_scale),
            (IDLE_DURATION, idle_scale),
            (BOUNCE_BACK_DURATION, bounce_back_inverse_scale),
            (REVEAL_DURATION, reveal_inverse_scale),
        ];

        let mut elapsed_time = simulation.time - self.start_time;

        for (duration, function) in steps {
            if elapsed_time <= duration {
                return function(elapsed_time);
            }

            elapsed_time -= duration;
        }

        0.0
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
        _shared_assets: &mut SharedBattleAssets,
        simulation: &mut BattleSimulation,
        _vms: &[RollbackVM],
    ) {
        if simulation.inputs.iter().any(|input| input.fleeing()) {
            // wait for the flee attempt to complete
            return;
        }

        if self.start_time == 0 {
            self.start_time = simulation.time;
            self.init_message(simulation);
        }

        self.complete = simulation.time - self.start_time >= MAX_DURATION;
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        // turn gauge
        simulation.turn_gauge.draw(sprite_queue);

        // text
        let mut style = TextStyle::new(game_io, FontStyle::Battle);
        style.letter_spacing = 0.0;
        style.scale.y = self.resolve_message_scale(simulation);

        let size = style.measure(&self.message).size;
        let position = (RESOLUTION_F - size * style.scale) * 0.5;
        style.bounds.set_position(position);

        style.draw(game_io, sprite_queue, &self.message);
    }
}
