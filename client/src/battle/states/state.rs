use crate::battle::{BattleSimulation, RollbackVM, SharedBattleAssets};
use crate::render::SpriteColorQueue;
use crate::resources::Globals;
use framework::prelude::GameIO;

pub trait State {
    fn clone_box(&self) -> Box<dyn State>;

    fn next_state(&self, game_io: &GameIO<Globals>) -> Option<Box<dyn State>>;

    fn update(
        &mut self,
        game_io: &GameIO<Globals>,
        shared_assets: &mut SharedBattleAssets,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    );

    fn draw_ui<'a>(
        &mut self,
        _game_io: &'a GameIO<Globals>,
        _simulation: &mut BattleSimulation,
        _sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
    }
}
