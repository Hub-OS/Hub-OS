use crate::overworld::components::*;
use crate::scenes::OverworldSceneBase;

pub fn system_update_animation(scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;

    for (_, animator) in entities.query_mut::<&mut Animator>() {
        animator.update();
    }
}

pub fn system_apply_animation(scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;

    for (_, (animator, sprite)) in entities.query_mut::<(&mut Animator, &mut Sprite)>() {
        animator.apply(sprite);
    }
}
