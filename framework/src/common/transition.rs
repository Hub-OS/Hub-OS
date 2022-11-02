use crate::prelude::*;

pub trait Transition<Globals> {
    fn draw(
        &mut self,
        game_io: &mut GameIO<Globals>,
        render_pass: &mut RenderPass,
        previous_scene: &mut Box<dyn Scene<Globals>>,
        next_scene: &mut Box<dyn Scene<Globals>>,
    );

    fn is_complete(&self) -> bool;
}
