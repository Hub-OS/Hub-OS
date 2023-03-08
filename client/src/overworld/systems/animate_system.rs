use crate::overworld::components::*;
use crate::overworld::OverworldArea;

pub fn system_update_animation(area: &mut OverworldArea) {
    let entities = &mut area.entities;

    for (_, animator) in entities.query_mut::<&mut Animator>() {
        animator.update();
    }
}

pub fn system_apply_animation(area: &mut OverworldArea) {
    let entities = &mut area.entities;

    for (_, (animator, sprite)) in entities.query_mut::<(&mut Animator, &mut Sprite)>() {
        animator.apply(sprite);
    }
}
