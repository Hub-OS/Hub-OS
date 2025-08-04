use crate::overworld::components::*;

pub fn system_update_animation(entities: &mut hecs::World) {
    for (_, animator) in entities.query_mut::<&mut Animator>() {
        animator.update();
    }
}

pub fn system_apply_animation(entities: &mut hecs::World) {
    for (_, (animator, sprite)) in entities.query_mut::<(&mut Animator, &mut Sprite)>() {
        animator.apply(sprite);
    }
}
