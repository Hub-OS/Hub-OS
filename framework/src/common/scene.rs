use crate::prelude::*;

pub trait Scene<Globals> {
    /// Engine accessor to the variable storing NextScene
    fn next_scene(&mut self) -> &mut NextScene<Globals>;

    /// Called when this scene is processed as the NextScene
    fn enter(&mut self, _game_io: &mut GameIO<Globals>) {}

    /// Called when a scene is popped and the transition is completed or None
    fn resume(&mut self, _game_io: &mut GameIO<Globals>) {}

    /// Called every tick, put game logic here
    fn update(&mut self, game_io: &mut GameIO<Globals>);

    /// Called to perform rendering. Not guaranteed to be called after every update
    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass);
}
