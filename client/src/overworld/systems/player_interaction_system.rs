use crate::overworld::components::*;
use crate::overworld::*;
use crate::resources::*;
use crate::scenes::OverworldSceneBase;
use framework::prelude::*;

pub fn system_player_interaction(game_io: &mut GameIO<Globals>, scene: &mut OverworldSceneBase) {
    let player_data = &mut scene.player_data;

    player_data.actor_interaction = None;
    player_data.object_interaction = None;
    player_data.tile_interaction = None;

    if scene.is_input_locked(game_io) {
        return;
    }

    let player_data = &mut scene.player_data;
    let entities = &mut scene.entities;
    let map = &scene.map;

    let input_util = InputUtil::new(game_io);

    if !input_util.was_just_pressed(Input::Confirm)
        && !input_util.was_just_pressed(Input::ShoulderL)
    {
        return;
    }

    const RANGE: f32 = 4.0;

    let (position, direction, collider) = entities
        .query_one_mut::<(&Vec3, &Direction, &ActorCollider)>(player_data.entity)
        .unwrap();

    let unit_vector: Vec2 = map
        .screen_direction_to_world(*direction)
        .unit_vector()
        .into();

    let interaction_point = *position + unit_vector.extend(0.0) * (collider.radius + RANGE);

    // test objects
    if let Some(entity) = map.tile_object_at(interaction_point.xy(), position.z as i32, true) {
        let data = map.object_entities().get::<&ObjectData>(entity).unwrap();
        player_data.object_interaction = Some(data.id);
    }

    // test actors
    let query = entities.query_mut::<(&Vec3, &ActorCollider, &InteractableActor)>();

    for (_, (position, ActorCollider { radius, .. }, InteractableActor(id))) in query {
        if (interaction_point.z - position.z).abs() >= 1.0 {
            // a whole layer apart, just skip
            continue;
        }

        let dist_squared = position.xy().distance_squared(interaction_point.xy());

        if dist_squared > *radius * *radius {
            // point was not in the collision radius
            continue;
        }

        player_data.actor_interaction = Some(id.clone());
        break;
    }

    // set tile interaction
    player_data.tile_interaction = Some(map.world_3d_to_tile_space(interaction_point));
}
