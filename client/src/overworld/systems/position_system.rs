use crate::overworld::components::*;
use crate::overworld::OverworldArea;

pub fn system_position(area: &mut OverworldArea) {
    let entities = &mut area.entities;
    let map = &area.map;

    for (_, (world_position, sprite)) in entities.query_mut::<(&Vec3, &mut Sprite)>() {
        let position = map.world_3d_to_screen(*world_position).floor();
        sprite.set_position(position);
    }
}
