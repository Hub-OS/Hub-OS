use crate::battle::{BattleSimulation, SharedBattleResources};
use crate::render::SpriteColorQueue;
use framework::prelude::GameIO;

pub trait State {
    fn clone_box(&self) -> Box<dyn State>;

    fn next_state(&self, game_io: &GameIO) -> Option<Box<dyn State>>;

    fn allows_animation_updates(&self) -> bool {
        false
    }

    fn update(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    );

    fn draw_ui<'a>(
        &mut self,
        _game_io: &'a GameIO,
        _simulation: &mut BattleSimulation,
        _sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
    }
}
