use crate::overworld::components::ActorPropertyAnimator;
use crate::overworld::OverworldArea;
use crate::resources::AssetManager;
use framework::prelude::GameIO;

pub fn system_actor_property_animation(
    game_io: &GameIO,
    assets: &impl AssetManager,
    area: &mut OverworldArea,
) {
    ActorPropertyAnimator::update(game_io, assets, &mut area.entities);
}
