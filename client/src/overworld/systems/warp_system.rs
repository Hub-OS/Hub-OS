use crate::overworld::components::{WarpController, WarpEffect};
use crate::overworld::{ObjectData, ObjectType, OverworldBaseEvent};
use crate::resources::Globals;
use crate::scenes::{InitialConnectScene, OverworldSceneBase};
use crate::transitions::{ColorFadeTransition, DEFAULT_FADE_DURATION};
use framework::prelude::*;
use packets::structures::Direction;

pub fn system_warp(game_io: &GameIO<Globals>, scene: &mut OverworldSceneBase) {
    let player_data = &scene.player_data;
    let entities = &mut scene.entities;
    let map = &mut scene.map;

    let (position, direction, warp_controller) = entities
        .query_one_mut::<(&mut Vec3, &Direction, &WarpController)>(player_data.entity)
        .unwrap();

    if warp_controller.warp_entity.is_some() {
        return;
    }

    let (position, direction) = (*position, *direction);

    let Some(entity) = map.tile_object_at(position.xy(), position.z as i32, false) else {
      return;
    };

    let data = map
        .object_entities_mut()
        .query_one_mut::<&ObjectData>(entity)
        .unwrap();

    if !data.object_type.is_warp() {
        return;
    }

    match data.object_type {
        ObjectType::CustomWarp | ObjectType::CustomServerWarp => {
            let callback = move |_: &GameIO<Globals>, scene: &mut OverworldSceneBase| {
                let event = OverworldBaseEvent::PendingWarp { entity };
                scene.events.push_back(event);
            };

            WarpEffect::warp_out(game_io, scene, player_data.entity, callback);
        }
        ObjectType::ServerWarp => {
            let address = data.custom_properties.get("address").to_string();
            let data = data.custom_properties.get("data").to_string();
            let data = if data.is_empty() { None } else { Some(data) };

            let callback = move |game_io: &GameIO<Globals>, scene: &mut OverworldSceneBase| {
                // todo: use push and fix infinite warp issues
                scene.next_scene =
                    NextScene::new_swap(InitialConnectScene::new(game_io, address, data, false))
                        .with_transition(ColorFadeTransition::new(
                            game_io,
                            Color::WHITE,
                            DEFAULT_FADE_DURATION,
                        ));
            };

            WarpEffect::warp_out(game_io, scene, player_data.entity, callback);
        }
        ObjectType::PositionWarp => {
            let target_position = Vec3::new(
                data.custom_properties.get_f32("x"),
                data.custom_properties.get_f32("y"),
                data.custom_properties.get_f32("z"),
            );

            WarpEffect::warp_full(
                game_io,
                scene,
                player_data.entity,
                target_position,
                direction,
                |_, _| {},
            );
        }
        ObjectType::HomeWarp => {
            let callback = |game_io: &GameIO<Globals>, scene: &mut OverworldSceneBase| {
                let transition =
                    ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION);
                scene.next_scene = NextScene::new_pop().with_transition(transition);
            };

            WarpEffect::warp_out(game_io, scene, player_data.entity, callback);
        }
        _ => {}
    }
}
