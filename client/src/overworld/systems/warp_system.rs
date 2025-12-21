use crate::overworld::components::{WarpController, WarpEffect};
use crate::overworld::{AutoEmote, ObjectData, ObjectType, OverworldArea, OverworldEvent};
use crate::resources::{Globals, Network, ServerStatus};
use framework::prelude::*;
use packets::structures::Direction;

pub fn system_warp(game_io: &mut GameIO, area: &mut OverworldArea) {
    let player_data = &area.player_data;
    let entities = &mut area.entities;
    let map = &mut area.map;

    let (position, direction, warp_controller) = entities
        .query_one_mut::<(&mut Vec3, &Direction, &WarpController)>(player_data.entity)
        .unwrap();

    if warp_controller.warped_out {
        return;
    }

    let (position, direction) = (*position, *direction);

    let Some(entity) = map.tile_object_at(position.xy(), position.z as i32, false) else {
        return;
    };

    let object_data = map
        .object_entities_mut()
        .query_one_mut::<&ObjectData>(entity)
        .unwrap();

    if !object_data.object_type.is_warp() {
        return;
    }

    match object_data.object_type {
        ObjectType::CustomWarp | ObjectType::CustomServerWarp => {
            let callback = move |_: &mut GameIO, area: &mut OverworldArea| {
                let event = OverworldEvent::PendingWarp { entity };
                area.event_sender.send(event).unwrap();
            };

            WarpEffect::warp_out(game_io, area, player_data.entity, callback);
        }
        ObjectType::ServerWarp => {
            let address = object_data.custom_properties.get("address").to_string();
            let data = object_data.custom_properties.get("data").to_string();
            let data = if data.is_empty() { None } else { Some(data) };
            let direction: Direction = object_data.custom_properties.get("direction").into();

            let mut warp_position = object_data.position.extend(position.z);
            warp_position.x += object_data.size.y * 0.5;
            warp_position.y += object_data.size.y * 0.5;

            // begin poll task
            let globals = game_io.resource_mut::<Globals>().unwrap();
            let subscription = globals.network.subscribe_to_server(address.clone());

            let poll_task = game_io.spawn_local_task(async move {
                let Some((send, receiver)) = subscription.await else {
                    return false;
                };

                Network::poll_server(&send, &receiver).await == ServerStatus::Online
            });

            let callback = move |game_io: &mut GameIO, area: &mut OverworldArea| {
                let event_sender = area.event_sender.clone();
                let player_entity = area.player_data.entity;

                let globals = game_io.resource::<Globals>().unwrap();
                let fail_message = globals.translate("server-warp-failed");

                game_io
                    .spawn_local_task(async move {
                        let events = if poll_task.await {
                            vec![OverworldEvent::TransferServer { address, data }]
                        } else {
                            vec![
                                OverworldEvent::SystemMessage {
                                    message: fail_message,
                                },
                                OverworldEvent::WarpIn {
                                    target_entity: player_entity,
                                    position: warp_position,
                                    direction,
                                },
                            ]
                        };

                        for event in events {
                            event_sender.send(event).unwrap();
                        }
                    })
                    .detach();
            };

            WarpEffect::warp_out(game_io, area, player_data.entity, callback);
        }
        ObjectType::PositionWarp => {
            let target_position = Vec3::new(
                object_data.custom_properties.get_f32("x"),
                object_data.custom_properties.get_f32("y"),
                object_data.custom_properties.get_f32("z"),
            );

            WarpEffect::warp_full(
                game_io,
                area,
                player_data.entity,
                target_position,
                direction,
                |_, _| {},
            );
        }
        ObjectType::HomeWarp => {
            let callback = |game_io: &mut GameIO, area: &mut OverworldArea| {
                let transition = crate::transitions::new_connect(game_io);

                area.event_sender
                    .send(OverworldEvent::NextScene((
                        AutoEmote::None,
                        NextScene::new_pop().with_transition(transition),
                    )))
                    .unwrap();
            };

            WarpEffect::warp_out(game_io, area, player_data.entity, callback);
        }
        _ => {}
    }
}
