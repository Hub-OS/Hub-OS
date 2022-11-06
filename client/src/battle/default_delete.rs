use super::{BattleCallback, BattleSimulation, Component, Entity, Living};
use crate::bindable::EntityID;
use crate::resources::{Globals, ResourcePaths};
use framework::prelude::GameIO;

pub fn delete_player(game_io: &GameIO<Globals>, simulation: &mut BattleSimulation, id: EntityID) {
    let player_deleted_sfx = &game_io.globals().player_deleted_sfx;
    simulation.play_sound(game_io, player_deleted_sfx);

    let (entity, living) = simulation
        .entities
        .query_one_mut::<(&mut Entity, &Living)>(id.into())
        .unwrap();

    let x = entity.x;
    let y = entity.y;

    // flinch
    let player_animator = &mut simulation.animators[entity.animator_index];
    let callbacks = player_animator.set_state(living.flinch_anim_state.as_ref().unwrap());
    simulation.pending_callbacks.extend(callbacks);

    let player_root_node = entity.sprite_tree.root_mut();
    player_animator.apply(player_root_node);

    player_animator.disable();

    // create transformation shine artifact
    Component::new_player_deletion(simulation, id);

    let artifact_id = simulation.create_artifact(game_io);
    let artifact_entity = simulation
        .entities
        .query_one_mut::<&mut Entity>(artifact_id.into())
        .unwrap();

    artifact_entity.x = x;
    artifact_entity.y = y;
    artifact_entity.pending_spawn = true;

    let root_node = artifact_entity.sprite_tree.root_mut();
    root_node.set_texture(game_io, ResourcePaths::BATTLE_TRANSFORM_SHINE.to_string());

    let animator = &mut simulation.animators[artifact_entity.animator_index];
    animator.load(game_io, ResourcePaths::BATTLE_TRANSFORM_SHINE_ANIMATION);
    let _ = animator.set_state("DEFAULT");

    animator.on_complete(BattleCallback::new(move |_, simulation, _, _| {
        let artifact_entity = simulation
            .entities
            .query_one_mut::<&mut Entity>(artifact_id.into())
            .unwrap();

        artifact_entity.erased = true;
    }));
}

pub fn delete_character(simulation: &mut BattleSimulation, id: EntityID) {
    Component::new_character_deletion(simulation, id);
}
