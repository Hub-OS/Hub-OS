use crate::overworld::components::*;
use crate::scenes::OverworldSceneBase;

pub fn system_position(scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;
    let map = &scene.map;

    for (_, (world_position, sprite)) in entities.query_mut::<(&Vec3, &mut Sprite)>() {
        let position = map.world_3d_to_screen(*world_position);
        sprite.set_position(position);
    }
}
