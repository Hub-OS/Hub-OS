use crate::overworld::components::*;
use crate::overworld::*;
use crate::render::SpriteColorQueue;
use crate::resources::*;
use crate::scenes::OverworldSceneBase;
use framework::prelude::*;

const RANGE_EXTENSION: f32 = 4.0;

pub fn system_player_interaction(game_io: &mut GameIO, scene: &mut OverworldSceneBase) {
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

    let interaction_point = calculate_interaction_point(map, entities, player_data.entity);

    // test objects
    let layer_index = interaction_point.z as i32;
    if let Some(entity) = map.tile_object_at(interaction_point.xy(), layer_index, true) {
        let data = map.object_entities().get::<&ObjectData>(entity).unwrap();
        player_data.object_interaction = Some(data.id);
    }

    // test actors
    let query = entities.query_mut::<(&Vec3, &ActorCollider, &InteractableActor)>();

    for (_, (position, &ActorCollider { radius, .. }, InteractableActor(id))) in query {
        if (interaction_point.z - position.z).abs() >= 1.0 {
            // a whole layer apart, just skip
            continue;
        }

        let dist_squared = position.xy().distance_squared(interaction_point.xy());

        if dist_squared > radius * radius {
            // point was not in the collision radius
            continue;
        }

        player_data.actor_interaction = Some(id.clone());
        break;
    }

    // set tile interaction
    player_data.tile_interaction = Some(map.world_3d_to_tile_space(interaction_point));
}

fn calculate_interaction_point(map: &Map, entities: &hecs::World, entity: hecs::Entity) -> Vec3 {
    let mut query = entities
        .query_one::<(&Vec3, &Direction, &ActorCollider)>(entity)
        .unwrap();
    let (&position, direction, collider) = query.get().unwrap();

    let unit_vector: Vec2 = map
        .screen_direction_to_world(*direction)
        .unit_vector()
        .into();

    position + unit_vector.extend(0.0) * (collider.radius + RANGE_EXTENSION)
}

pub fn player_interaction_debug_render(
    game_io: &GameIO,
    scene: &OverworldSceneBase,
    sprite_queue: &mut SpriteColorQueue,
) {
    let globals = game_io.resource::<Globals>().unwrap();

    if !globals.debug_visible {
        return;
    }

    let entity = scene.player_data.entity;
    let interaction_point = calculate_interaction_point(&scene.map, &scene.entities, entity);
    let position = scene.map.world_3d_to_screen(interaction_point);

    let assets = &globals.assets;
    let mut pixel = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
    pixel.set_origin(Vec2::ONE * 0.5);
    pixel.set_position(position);

    // render doubled size in white for border
    pixel.set_scale(Vec2::ONE * 2.0);
    sprite_queue.draw_sprite(&pixel);

    // render normal size in red for point
    pixel.set_color(Color::RED);
    pixel.set_scale(Vec2::ONE);
    sprite_queue.draw_sprite(&pixel);
}
