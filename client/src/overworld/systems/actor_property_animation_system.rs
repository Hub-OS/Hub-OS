use crate::overworld::components::ActorPropertyAnimator;
use crate::resources::AssetManager;
use crate::scenes::OverworldSceneBase;
use framework::prelude::GameIO;

pub fn system_actor_property_animation(
    game_io: &GameIO,
    assets: &impl AssetManager,
    scene: &mut OverworldSceneBase,
) {
    ActorPropertyAnimator::update(game_io, assets, &mut scene.entities);
}
