use crate::overworld::components::*;
use crate::scenes::OverworldSceneBase;

pub fn system_animate(scene: &mut OverworldSceneBase) {
    let entities = &mut scene.entities;

    for (_, (animator, sprite)) in entities.query_mut::<(&mut Animator, &mut Sprite)>() {
        animator.update();
        animator.apply(sprite);
    }
}
