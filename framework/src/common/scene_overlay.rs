use crate::prelude::*;

pub trait SceneOverlay<Globals> {
    /// Called every tick, put game logic here
    fn update(&mut self, game_io: &mut GameIO<Globals>);

    /// Called to perform rendering. Not guaranteed to be called after every update
    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass);
}
