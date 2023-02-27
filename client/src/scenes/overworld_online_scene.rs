use super::{InitialConnectScene, NetplayInitScene, NetplayProps, OverworldSceneBase};
use crate::battle::BattleProps;
use crate::bindable::Emotion;
use crate::overworld::{components::*, system_actor_property_animation};
use crate::overworld::{
    system_movement_interpolation, CameraAction, Identity, Item, ObjectData, ObjectType,
    OverworldEvent, ServerAssetManager,
};
use crate::packages::{PackageId, PackageNamespace};
use crate::render::ui::{
    TextboxDoorstop, TextboxDoorstopRemover, TextboxInterface, TextboxMessage, TextboxPrompt,
    TextboxQuestion, TextboxQuiz,
};
use crate::render::AnimatorLoopMode;
use crate::resources::*;
use crate::saves::BlockGrid;
use crate::scenes::BattleScene;
use bimap::BiMap;
use framework::prelude::*;
use packets::structures::{ActorProperty, BattleStatistics, FileHash};
use packets::{
    address_parsing, ClientAssetType, ClientPacket, Reliability, ServerPacket, SERVER_TICK_RATE,
};
use std::collections::{HashMap, VecDeque};

pub struct OverworldOnlineScene {
    base_scene: OverworldSceneBase,
    next_scene: NextScene,
    next_scene_queue: VecDeque<NextScene>,
    connected: bool,
    transferring: bool,
    identity: Identity,
    server_address: String,
    send_packet: ClientPacketSender,
    packet_receiver: ServerPacketReceiver,
    synchronizing_packets: bool,
    stored_packets: Vec<ServerPacket>,
    previous_boost_packet: Option<ClientPacket>,
    last_position_send: Instant,
    assets: ServerAssetManager,
    custom_emotes_path: String,
    actor_id_map: BiMap<String, hecs::Entity>,
    excluded_actors: Vec<String>,
    excluded_objects: Vec<u32>,
    doorstop_remover: Option<TextboxDoorstopRemover>,
    encounter_packages: HashMap<String, PackageId>, // server_path -> package_id
    loaded_zips: HashMap<String, FileHash>,         // server_path -> hash
}

impl OverworldOnlineScene {
    pub fn new(
        game_io: &mut GameIO,
        address: String,
        send_packet: ClientPacketSender,
        packet_receiver: flume::Receiver<ServerPacket>,
    ) -> Self {
        let mut base_scene = OverworldSceneBase::new(game_io);

        let player_entity = base_scene.player_data.entity;
        let entities = &mut base_scene.entities;
        entities
            .insert_one(player_entity, Excluded::default())
            .unwrap();

        let assets = ServerAssetManager::new(game_io, &address);

        Self {
            base_scene,
            next_scene: NextScene::None,
            next_scene_queue: VecDeque::new(),
            connected: true,
            transferring: false,
            identity: Identity::for_address(&address),
            server_address: address,
            send_packet,
            packet_receiver,
            synchronizing_packets: false,
            stored_packets: Vec::new(),
            previous_boost_packet: None,
            last_position_send: game_io.frame_start_instant(),
            assets,
            custom_emotes_path: ResourcePaths::BLANK.to_string(),
            actor_id_map: BiMap::new(),
            excluded_actors: Vec::new(),
            excluded_objects: Vec::new(),
            doorstop_remover: None,
            encounter_packages: HashMap::new(),
            loaded_zips: HashMap::new(),
        }
    }

    pub fn start_connection(&mut self, game_io: &GameIO, data: Option<String>) {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;

        let send_packet = &self.send_packet;

        let data =
            data.unwrap_or_else(|| address_parsing::slice_data(&self.server_address).to_string());

        // send login packet
        send_packet(
            Reliability::ReliableOrdered,
            ClientPacket::Login {
                username: global_save.nickname.clone(),
                identity: self.identity.data().to_vec(),
                data,
            },
        );

        // send asset found signals
        for asset in self.assets.stored_assets() {
            send_packet(
                Reliability::ReliableOrdered,
                ClientPacket::AssetFound {
                    path: asset.remote_path,
                    last_modified: asset.last_modified,
                },
            );
        }

        // send boosts
        self.send_boosts(game_io);

        // send avatar data
        self.send_avatar_data(game_io);

        // nothing else to send, request join
        let send_packet = &self.send_packet;
        send_packet(Reliability::ReliableOrdered, ClientPacket::RequestJoin);
    }

    pub fn send_boosts(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;

        let blocks = global_save.active_blocks().cloned().unwrap_or_default();
        let block_grid = BlockGrid::new(PackageNamespace::Server).with_blocks(game_io, blocks);

        let packet = ClientPacket::Boost {
            health_boost: self.base_scene.player_data.health_boost,
            augments: block_grid
                .augments(game_io)
                .map(|(package, _)| package.package_info.id.clone())
                .collect(),
        };

        if self.previous_boost_packet.as_ref() == Some(&packet) {
            return;
        }

        self.previous_boost_packet = Some(packet.clone());

        let send_packet = &self.send_packet;
        send_packet(Reliability::ReliableOrdered, packet);
    }

    pub fn send_avatar_data(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;
        let player_package = global_save.player_package(game_io).unwrap();

        let assets = &globals.assets;
        let send_packet = &self.send_packet;

        vec![
            ClientPacket::Asset {
                asset_type: ClientAssetType::Texture,
                data: assets.binary(&player_package.overworld_texture_path),
            },
            ClientPacket::Asset {
                asset_type: ClientAssetType::Animation,
                data: assets.binary(&player_package.overworld_animation_path),
            },
            ClientPacket::Asset {
                asset_type: ClientAssetType::MugshotTexture,
                data: assets.binary(&player_package.mugshot_texture_path),
            },
            ClientPacket::Asset {
                asset_type: ClientAssetType::MugshotAnimation,
                data: assets.binary(&player_package.mugshot_animation_path),
            },
        ]
        .into_iter()
        .for_each(|packet| send_packet(Reliability::ReliableOrdered, packet));

        // send avatar change signal
        send_packet(
            Reliability::ReliableOrdered,
            ClientPacket::AvatarChange {
                name: player_package.name.clone(),
                element: player_package.element.to_string(),
                base_health: player_package.health,
            },
        );
    }

    pub fn packet_receiver(&self) -> &ServerPacketReceiver {
        &self.packet_receiver
    }

    pub fn handle_packets(&mut self, game_io: &mut GameIO) {
        if !self.connected {
            return;
        }

        while let Ok(packet) = self.packet_receiver.try_recv() {
            self.handle_packet(game_io, packet);
        }

        if self.packet_receiver.is_disconnected() {
            self.base_scene
                .event_sender
                .send(OverworldEvent::Disconnected {
                    message: String::from(
                        "Everything is still.\x01..\x01\nLooks like we've been disconnected.",
                    ),
                })
                .unwrap();
        }
    }

    pub fn handle_packet(&mut self, game_io: &mut GameIO, packet: ServerPacket) {
        if self.synchronizing_packets {
            self.stored_packets.push(packet);
            return;
        }

        #[allow(unused)]
        match packet {
            ServerPacket::VersionInfo { .. } => {
                // handled by initial_connect_scene.rs
            }
            ServerPacket::Heartbeat => {
                // no action required, server just wants an ack to keep connections alive on both sides
            }
            ServerPacket::Authorize { address, data } => {
                let globals = game_io.resource_mut::<Globals>().unwrap();
                let subscription = globals.network.subscribe_to_server(address);

                let origin_address = address_parsing::strip_data(&self.server_address).to_string();
                let identity = self.identity.data().to_vec();

                let fut = async move {
                    let (send, _) = subscription.await?;

                    send(
                        Reliability::ReliableOrdered,
                        ClientPacket::Authorize {
                            origin_address,
                            identity,
                            data,
                        },
                    );

                    Some(())
                };

                game_io.spawn_local_task(fut).detach()
            }
            ServerPacket::Login {
                actor_id,
                warp_in,
                spawn_x,
                spawn_y,
                spawn_z,
                spawn_direction,
            } => {
                let player_entity = self.base_scene.player_data.entity;
                self.actor_id_map.insert(actor_id, player_entity);

                let entities = &mut self.base_scene.entities;

                let map = &self.base_scene.map;
                let tile_position = Vec3::new(spawn_x, spawn_y, spawn_z);
                let spawn_position = map.tile_3d_to_world(tile_position);

                let (position, direction) = entities
                    .query_one_mut::<(&mut Vec3, &mut Direction)>(player_entity)
                    .unwrap();
                *position = spawn_position;
                *direction = spawn_direction;

                entities.remove_one::<Excluded>(player_entity);

                if warp_in {
                    WarpEffect::warp_in(
                        game_io,
                        &mut self.base_scene,
                        player_entity,
                        spawn_position,
                        spawn_direction,
                        |_, _| {},
                    );
                }
            }
            ServerPacket::CompleteConnection => {
                self.send_ready_packet(game_io);
            }
            ServerPacket::TransferWarp => {
                let player_entity = self.base_scene.player_data.entity;
                let send_packet = self.send_packet.clone();

                WarpEffect::warp_out(game_io, &mut self.base_scene, player_entity, move |_, _| {
                    send_packet(Reliability::ReliableOrdered, ClientPacket::TransferredOut);
                });
            }
            ServerPacket::TransferStart => {
                self.excluded_actors.clear();
                self.excluded_objects.clear();
                self.transferring = true;

                // despawn all other actors
                let player_entity = self.base_scene.player_data.entity;
                let (player_id, _) = self.actor_id_map.remove_by_right(&player_entity).unwrap();

                for entity in self.actor_id_map.right_values() {
                    let _ = self.base_scene.entities.despawn(*entity);
                }

                self.actor_id_map.clear();
                self.actor_id_map.insert(player_id, player_entity);
            }
            ServerPacket::TransferComplete { warp_in, direction } => {
                let player_entity = self.base_scene.player_data.entity;
                let send_packet = self.send_packet.clone();

                let entities = &mut self.base_scene.entities;
                let (position, current_direction) = entities
                    .query_one_mut::<(&Vec3, &mut Direction)>(player_entity)
                    .unwrap();

                if !direction.is_none() {
                    *current_direction = direction;
                }

                if warp_in {
                    let position = *position;
                    let direction = *current_direction;

                    WarpEffect::warp_in(
                        game_io,
                        &mut self.base_scene,
                        player_entity,
                        position,
                        direction,
                        move |_, _| {},
                    );
                }

                self.send_ready_packet(game_io);
                self.transferring = false;
            }
            ServerPacket::TransferServer {
                address,
                data,
                warp_out,
            } => {
                let player_entity = self.base_scene.player_data.entity;
                let data = if data.is_empty() { None } else { Some(data) };

                WarpEffect::warp_out(game_io, &mut self.base_scene, player_entity, |_, scene| {
                    let event = OverworldEvent::TransferServer { address, data };
                    scene.event_sender.send(event).unwrap();
                });
            }
            ServerPacket::Kick { reason } => {
                self.base_scene
                    .event_sender
                    .send(OverworldEvent::Disconnected {
                        message: format!("We've been kicked: \"{reason}\""),
                    })
                    .unwrap();
            }
            ServerPacket::RemoveAsset { path } => self.assets.delete_asset(&path),
            ServerPacket::AssetStreamStart {
                name,
                last_modified,
                cache_to_disk,
                data_type,
                size,
            } => {
                self.assets.start_download(
                    name,
                    last_modified,
                    size as usize,
                    data_type,
                    cache_to_disk,
                );
            }
            ServerPacket::AssetStream { data } => {
                self.assets.receive_download_data(game_io, data);
            }
            ServerPacket::Preload {
                asset_path,
                data_type,
            } => {
                use packets::structures::AssetDataType;

                match data_type {
                    AssetDataType::Data | AssetDataType::Text | AssetDataType::CompressedText => {
                        self.assets.binary(&asset_path);
                    }
                    AssetDataType::Texture => {
                        self.assets.texture(game_io, &asset_path);
                    }
                    AssetDataType::Audio => {
                        self.assets.audio(&asset_path);
                    }
                    AssetDataType::Unknown => {
                        log::warn!("Server sent unknown AssetType? {:?}", asset_path);
                    }
                }
            }
            ServerPacket::CustomEmotesPath { asset_path } => {
                self.custom_emotes_path = asset_path;
            }
            ServerPacket::MapUpdate { map_path } => {
                use crate::overworld::load_map;

                let data = self.assets.text(&map_path);

                if let Some(mut map) = load_map(game_io, &self.assets, &data) {
                    for id in &self.excluded_objects {
                        let Some(entity) = map.get_object_entity(*id) else {
                            continue;
                        };

                        Excluded::increment(map.object_entities_mut(), entity);
                    }

                    self.base_scene.set_world(game_io, &self.assets, map);
                } else {
                    log::warn!("Failed to load map provided by server");
                }
            }
            ServerPacket::Health { health } => {
                let player_data = &mut self.base_scene.player_data;
                player_data.health = health;

                self.base_scene.menu_manager.update_player_data(player_data);
            }
            ServerPacket::BaseHealth { base_health } => {
                let player_data = &mut self.base_scene.player_data;
                player_data.base_health = base_health;

                self.base_scene.menu_manager.update_player_data(player_data);
            }
            ServerPacket::Emotion { emotion } => {
                use num_traits::FromPrimitive;

                if let Some(emotion) = Emotion::from_u8(emotion) {
                    self.base_scene.player_data.emotion = emotion;
                }
            }
            ServerPacket::Money { money } => {
                let player_data = &mut self.base_scene.player_data;
                player_data.money = money;

                self.base_scene.menu_manager.update_player_data(player_data);
            }
            ServerPacket::AddItem {
                id,
                name,
                description,
            } => {
                self.base_scene.player_data.items.push(Item {
                    id,
                    name,
                    description,
                });
            }
            ServerPacket::RemoveItem { id } => {
                let items = &mut self.base_scene.player_data.items;

                if let Some(index) = items.iter().position(|item| item.id == id) {
                    items.remove(index);
                }
            }
            ServerPacket::PlaySound { path } => {
                let sound = self.assets.audio(&path);
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&sound);
            }
            ServerPacket::ExcludeObject { id } => {
                if !self.excluded_objects.contains(&id) {
                    let map = &mut self.base_scene.map;

                    if let Some(entity) = map.get_object_entity(id) {
                        let object_entities = map.object_entities_mut();

                        Excluded::increment(object_entities, entity);
                    }

                    self.excluded_objects.push(id);
                }
            }
            ServerPacket::IncludeObject { id } => {
                if let Some(index) = self.excluded_objects.iter().position(|v| *v == id) {
                    let map = &mut self.base_scene.map;

                    if let Some(entity) = map.get_object_entity(id) {
                        let object_entities = map.object_entities_mut();

                        Excluded::decrement(object_entities, entity);
                    }

                    self.excluded_objects.remove(index);
                }
            }
            ServerPacket::ExcludeActor { actor_id } => {
                if !self.excluded_actors.contains(&actor_id) {
                    if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                        let entities = &mut self.base_scene.entities;
                        Excluded::increment(entities, *entity);
                    }

                    self.excluded_actors.push(actor_id);
                }
            }
            ServerPacket::IncludeActor { actor_id } => {
                if let Some(index) = self.excluded_actors.iter().position(|v| *v == actor_id) {
                    if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                        let entities = &mut self.base_scene.entities;
                        Excluded::decrement(entities, *entity);
                    }

                    self.excluded_actors.remove(index);
                }
            }
            ServerPacket::MoveCamera {
                x,
                y,
                z,
                hold_duration,
            } => {
                let world_position = self.base_scene.map.tile_3d_to_world(Vec3 { x, y, z });
                let target = self.base_scene.map.world_3d_to_screen(world_position);

                self.base_scene.queue_camera_action(CameraAction::Snap {
                    target,
                    hold_duration,
                });
            }
            ServerPacket::SlideCamera { x, y, z, duration } => {
                let world_position = self.base_scene.map.tile_3d_to_world(Vec3 { x, y, z });
                let target = self.base_scene.map.world_3d_to_screen(world_position);

                self.base_scene
                    .queue_camera_action(CameraAction::Slide { target, duration });
            }
            ServerPacket::ShakeCamera { strength, duration } => {
                self.base_scene
                    .queue_camera_action(CameraAction::Shake { strength, duration });
            }
            ServerPacket::FadeCamera { color, duration } => {
                self.base_scene.queue_camera_action(CameraAction::Fade {
                    color: color.into(),
                    duration,
                });
            }
            ServerPacket::TrackWithCamera { actor_id } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id).cloned() {
                    self.base_scene
                        .queue_camera_action(CameraAction::TrackEntity { entity });
                }
            }
            ServerPacket::EnableCameraControls { dist_x, dist_y } => {
                log::warn!("EnableCameraControls hasn't been implemented")
            }
            ServerPacket::UnlockCamera => {
                self.base_scene.queue_camera_action(CameraAction::Unlock);
            }
            ServerPacket::EnableCameraZoom => {
                log::warn!("EnableCameraZoom hasn't been implemented")
            }
            ServerPacket::DisableCameraZoom => {
                log::warn!("DisableCameraZoom hasn't been implemented")
            }
            ServerPacket::LockInput => {
                self.base_scene.add_input_lock();
            }
            ServerPacket::UnlockInput => {
                self.base_scene.remove_input_lock();
            }
            ServerPacket::Teleport {
                warp,
                x,
                y,
                z,
                direction,
            } => {
                let tile_position = Vec3 { x, y, z };
                let position = self.base_scene.map.tile_3d_to_world(tile_position);

                let player_entity = self.base_scene.player_data.entity;

                if warp {
                    WarpEffect::warp_full(
                        game_io,
                        &mut self.base_scene,
                        player_entity,
                        position,
                        direction,
                        |_, _| {},
                    );
                } else {
                    let entities = &mut self.base_scene.entities;
                    let (set_position, set_direction) = entities
                        .query_one_mut::<(&mut Vec3, &mut Direction)>(player_entity)
                        .unwrap();

                    *set_position = position;
                    *set_direction = direction;
                }
            }
            ServerPacket::Message {
                message,
                mug_texture_path,
                mug_animation_path,
            } => {
                self.base_scene.menu_manager.set_next_avatar(
                    game_io,
                    &self.assets,
                    &mug_texture_path,
                    &mug_animation_path,
                );

                let event_sender = self.base_scene.event_sender.clone();
                let message_interface = TextboxMessage::new(message).with_callback(move || {
                    event_sender
                        .send(OverworldEvent::TextboxResponse(0))
                        .unwrap();
                });

                self.push_server_textbox_interface(message_interface);
            }
            ServerPacket::Question {
                message,
                mug_texture_path,
                mug_animation_path,
            } => {
                self.base_scene.menu_manager.set_next_avatar(
                    game_io,
                    &self.assets,
                    &mug_texture_path,
                    &mug_animation_path,
                );

                let event_sender = self.base_scene.event_sender.clone();
                let question_interface = TextboxQuestion::new(message, move |response| {
                    event_sender
                        .send(OverworldEvent::TextboxResponse(response as u8))
                        .unwrap();
                });

                self.push_server_textbox_interface(question_interface);
            }
            ServerPacket::Quiz {
                option_a,
                option_b,
                option_c,
                mug_texture_path,
                mug_animation_path,
            } => {
                self.base_scene.menu_manager.set_next_avatar(
                    game_io,
                    &self.assets,
                    &mug_texture_path,
                    &mug_animation_path,
                );

                let options: &[&str; 3] = &[&option_a, &option_b, &option_c];

                let event_sender = self.base_scene.event_sender.clone();
                let quiz_interface = TextboxQuiz::new(options, move |response| {
                    event_sender
                        .send(OverworldEvent::TextboxResponse(response as u8))
                        .unwrap();
                });

                self.push_server_textbox_interface(quiz_interface);
            }
            ServerPacket::Prompt {
                character_limit,
                default_text,
            } => {
                let event_sender = self.base_scene.event_sender.clone();
                let prompt_interface = TextboxPrompt::new(move |response| {
                    event_sender
                        .send(OverworldEvent::PromptResponse(response))
                        .unwrap();
                });

                self.push_server_textbox_interface(prompt_interface);
            }
            ServerPacket::TextBoxResponseAck => {
                if self.base_scene.menu_manager.remaining_textbox_interfaces() == 1 {
                    // big assumption here
                    // the last interface should be from the server
                    // todo: replace with a counter?
                    if let Some(remove) = self.doorstop_remover.take() {
                        remove();
                    }
                }
            }
            ServerPacket::OpenBoard {
                topic,
                color,
                posts,
                open_instantly,
            } => {
                let send_packet = self.send_packet.clone();

                // let the server know post selections from now on are for this board
                send_packet(Reliability::ReliableOrdered, ClientPacket::BoardOpen);

                let on_select = {
                    let send_packet = self.send_packet.clone();

                    move |id: &str| {
                        send_packet(
                            Reliability::ReliableOrdered,
                            ClientPacket::PostSelection {
                                post_id: id.to_string(),
                            },
                        );
                    }
                };

                let on_close = {
                    let send_packet = self.send_packet.clone();

                    move || {
                        send_packet(Reliability::ReliableOrdered, ClientPacket::BoardClose);
                    }
                };

                self.base_scene.menu_manager.open_bbs(
                    game_io,
                    topic,
                    color.into(),
                    !open_instantly,
                    on_select,
                    on_close,
                );

                if let Some(bbs) = self.base_scene.menu_manager.bbs_mut() {
                    bbs.append_posts(None, posts);
                }
            }
            ServerPacket::PrependPosts { reference, posts } => {
                if let Some(bbs) = self.base_scene.menu_manager.bbs_mut() {
                    bbs.prepend_posts(reference.as_deref(), posts);
                }
            }
            ServerPacket::AppendPosts { reference, posts } => {
                if let Some(bbs) = self.base_scene.menu_manager.bbs_mut() {
                    bbs.append_posts(reference.as_deref(), posts);
                }
            }
            ServerPacket::RemovePost { id } => {
                if let Some(bbs) = self.base_scene.menu_manager.bbs_mut() {
                    bbs.remove_post(&id);
                }
            }
            ServerPacket::PostSelectionAck => {
                self.base_scene.menu_manager.acknowledge_selection();
            }
            ServerPacket::CloseBBS => {
                if let Some(bbs) = self.base_scene.menu_manager.bbs_mut() {
                    bbs.close();
                }
            }
            ServerPacket::ShopInventory { items } => {
                log::warn!("ShopInventory hasn't been implemented")
            }
            ServerPacket::OpenShop {
                mug_texture_path,
                mug_animation_path,
            } => log::warn!("OpenShop hasn't been implemented"),
            ServerPacket::OfferPackage {
                id,
                name,
                category,
                package_path,
            } => log::warn!("OfferPackage hasn't been implemented"),
            ServerPacket::LoadPackage {
                category,
                package_path,
            } => {
                let namespace = PackageNamespace::Server;

                let globals = game_io.resource::<Globals>().unwrap();
                let assets = &globals.assets;

                let hash = match self.loaded_zips.get(&package_path) {
                    Some(hash) => *hash,
                    None => {
                        let bytes = self.assets.binary(&package_path);
                        let hash = FileHash::hash(&bytes);

                        assets.load_virtual_zip(game_io, hash, bytes);
                        self.loaded_zips.insert(package_path.clone(), hash);

                        hash
                    }
                };

                let globals = game_io.resource_mut::<Globals>().unwrap();

                if let Some(package_info) = globals.load_virtual_package(category, namespace, hash)
                {
                    // save package id for starting encounters
                    self.encounter_packages
                        .insert(package_path, package_info.id.clone());
                }
            }
            ServerPacket::ModWhitelist { whitelist_path } => {
                log::warn!("ModWhitelist hasn't been implemented")
            }
            ServerPacket::ModBlacklist { blacklist_path } => {
                log::warn!("ModBlacklist hasn't been implemented")
            }
            ServerPacket::InitiateEncounter {
                package_path,
                data,
                health,
                base_health,
            } => {
                let globals = game_io.resource::<Globals>().unwrap();

                if let Some(package_id) = self.encounter_packages.get(&package_path) {
                    // play sfx
                    globals.audio.stop_music();
                    globals.audio.play_sound(&globals.battle_transition_sfx);

                    // get package
                    let battle_package = globals
                        .battle_packages
                        .package_or_fallback(PackageNamespace::Server, package_id);

                    let mut props = BattleProps::new_with_defaults(game_io, battle_package);
                    props.data = data;

                    let player_setup = &mut props.player_setups[0];
                    player_setup.health = health;
                    player_setup.base_health = base_health;

                    // callback
                    let event_sender = self.base_scene.event_sender.clone();
                    props.statistics_callback = Some(Box::new(move |statistics| {
                        let _ = event_sender.send(OverworldEvent::BattleStatistics(statistics));
                    }));

                    // copy background
                    props.background = self
                        .base_scene
                        .map
                        .background_properties()
                        .generate_background(game_io, &self.assets);

                    // create scene
                    let scene = BattleScene::new(game_io, props);
                    let transition = crate::transitions::new_battle(game_io);

                    let next_scene = NextScene::new_push(scene).with_transition(transition);
                    self.next_scene_queue.push_back(next_scene);
                }
            }
            ServerPacket::InitiateNetplay {
                package_path,
                data,
                remote_players,
                health,
                base_health,
            } => {
                (self.send_packet)(Reliability::ReliableOrdered, ClientPacket::EncounterStart);

                // play sfx
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.stop_music();
                globals.audio.play_sound(&globals.battle_transition_sfx);

                // copy background
                let background = self
                    .base_scene
                    .map
                    .background_properties()
                    .generate_background(game_io, &self.assets);

                // callback
                let event_sender = self.base_scene.event_sender.clone();
                let statistics_callback = Box::new(move |statistics| {
                    let _ = event_sender.send(OverworldEvent::BattleStatistics(statistics));
                });

                // get package
                let battle_package = package_path
                    .and_then(|path| self.encounter_packages.get(&path))
                    .map(|id| (PackageNamespace::Server, id.clone()));

                // create scene
                let props = NetplayProps {
                    background: Some(background),
                    battle_package,
                    data,
                    health,
                    base_health,
                    remote_players,
                    fallback_address: self.server_address.clone(),
                    statistics_callback: Some(statistics_callback),
                };

                let scene = NetplayInitScene::new(game_io, props);

                let transition = crate::transitions::new_battle(game_io);
                let next_scene = NextScene::new_push(scene).with_transition(transition);
                self.next_scene_queue.push_back(next_scene);
            }
            ServerPacket::ActorConnected {
                actor_id,
                name,
                texture_path,
                animation_path,
                direction: initial_direction,
                x,
                y,
                z,
                solid,
                warp_in,
                scale_x,
                scale_y,
                rotation,
                minimap_color,
                animation,
            } => {
                let tile_position = Vec3::new(x, y, z);
                let position = self.base_scene.map.tile_3d_to_world(tile_position);

                let entity = if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    if *entity != self.base_scene.player_data.entity {
                        log::warn!("server used an actor_id that's already in use: {actor_id:?}");
                    }

                    // reuse entity
                    *entity
                } else {
                    // spawn actor
                    let texture = self.assets.texture(game_io, &texture_path);
                    let animator = Animator::load_new(&self.assets, &animation_path);

                    let entity = self
                        .base_scene
                        .spawn_player_actor(game_io, texture, animator, position);

                    // mark as excluded as it was marked before spawn
                    if self.excluded_actors.contains(&actor_id) {
                        let entities = &mut self.base_scene.entities;
                        Excluded::increment(entities, entity)
                    }

                    entity
                };

                if entity != self.base_scene.player_data.entity {
                    let entities = &mut self.base_scene.entities;

                    // tweak existing properties
                    let (sprite, direction, collider, minimap_marker) = entities
                        .query_one_mut::<(
                            &mut Sprite,
                            &mut Direction,
                            &mut ActorCollider,
                            &mut PlayerMinimapMarker,
                        )>(entity)
                        .unwrap();

                    sprite.set_scale(Vec2::new(scale_x, scale_y));
                    sprite.set_rotation(rotation);
                    *direction = initial_direction;
                    collider.solid = solid;
                    minimap_marker.color = minimap_color.into();

                    // setup remote player specific components
                    let _ = entities.insert(
                        entity,
                        (
                            InteractableActor(actor_id.clone()),
                            MovementInterpolator::new(game_io, position, initial_direction),
                        ),
                    );

                    if let Some(state) = animation {
                        // setup initial animation
                        let (animator, movement_animator) = entities
                            .query_one_mut::<(&mut Animator, &mut MovementAnimator)>(entity)
                            .unwrap();

                        animator.set_state(&state);
                        movement_animator.set_animation_enabled(false);
                    }

                    if warp_in && !self.excluded_actors.contains(&actor_id) {
                        // create warp effect
                        WarpEffect::warp_in(
                            game_io,
                            &mut self.base_scene,
                            entity,
                            position,
                            initial_direction,
                            |_, _| {},
                        );
                    }

                    self.actor_id_map.insert(actor_id, entity);
                }
            }
            ServerPacket::ActorDisconnected { actor_id, warp_out } => {
                if let Some((_, entity)) = self.actor_id_map.remove_by_left(&actor_id) {
                    if warp_out {
                        let event_sender = self.base_scene.event_sender.clone();

                        WarpEffect::warp_out(
                            game_io,
                            &mut self.base_scene,
                            entity,
                            move |_, base_scene| {
                                let _ = base_scene.entities.despawn(entity);
                            },
                        );
                    } else {
                        let _ = self.base_scene.entities.despawn(entity);
                    }
                }
            }
            ServerPacket::ActorSetName { actor_id, name } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let entities = &mut self.base_scene.entities;

                    if let Ok(label) = entities.query_one_mut::<(&mut NameLabel)>(*entity) {
                        label.0 = name;
                    }
                }
            }
            ServerPacket::ActorMove {
                actor_id,
                x,
                y,
                z,
                direction,
            } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let entities = &mut self.base_scene.entities;

                    let animating_properties = entities
                        .satisfies::<(&ActorPropertyAnimator)>(*entity)
                        .unwrap_or(false);

                    if let Ok(interpolator) =
                        entities.query_one_mut::<(&mut MovementInterpolator)>(*entity)
                    {
                        let tile_position = Vec3::new(x, y, z);
                        let position = self.base_scene.map.tile_3d_to_world(tile_position);

                        if interpolator.is_movement_impossible(position) {
                            if !animating_properties && !self.excluded_actors.contains(&actor_id) {
                                WarpEffect::warp_full(
                                    game_io,
                                    &mut self.base_scene,
                                    *entity,
                                    position,
                                    direction,
                                    |_, _| {},
                                );
                            }
                        } else {
                            interpolator.push(game_io, position, direction);
                        }
                    }
                }
            }
            ServerPacket::ActorSetAvatar {
                actor_id,
                texture_path,
                animation_path,
            } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let entities = &mut self.base_scene.entities;
                    let (sprite, animator) = entities
                        .query_one_mut::<(&mut Sprite, &mut Animator)>(*entity)
                        .unwrap();

                    let texture = self.assets.texture(game_io, &texture_path);
                    sprite.set_texture(texture);
                    animator.load(&self.assets, &animation_path);
                }
            }
            ServerPacket::ActorEmote {
                actor_id,
                emote_id,
                use_custom_emotes,
            } => log::warn!("ActorEmote hasn't been implemented"),
            ServerPacket::ActorAnimate {
                actor_id,
                state,
                loop_animation,
            } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let entities = &mut self.base_scene.entities;

                    let (animator, movement_animator) = entities
                        .query_one_mut::<(&mut Animator, &mut MovementAnimator)>(*entity)
                        .unwrap();

                    animator.set_state(&state);

                    if loop_animation {
                        animator.set_loop_mode(AnimatorLoopMode::Loop);
                    }

                    movement_animator.set_animation_enabled(false);
                }
            }
            ServerPacket::ActorPropertyKeyFrames {
                actor_id,
                keyframes,
            } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let mut property_animator = ActorPropertyAnimator::new();

                    if *entity != self.base_scene.player_data.entity {
                        property_animator.set_audio_enabled(false);
                    }

                    let tile_size = self.base_scene.map.tile_size().as_vec2();

                    for mut keyframe in keyframes {
                        // fix scale
                        for (property, ease) in &mut keyframe.property_steps {
                            match property {
                                ActorProperty::X(value) => {
                                    *property = ActorProperty::X(*value * tile_size.x * 0.5);
                                }
                                ActorProperty::Y(value) => {
                                    *property = ActorProperty::Y(*value * tile_size.y);
                                }
                                _ => {}
                            }
                        }

                        property_animator.add_key_frame(keyframe);
                    }

                    // stop the previous animator
                    let entities = &mut self.base_scene.entities;
                    ActorPropertyAnimator::stop(entities, *entity);

                    // insert and start the new one
                    entities.insert_one(*entity, property_animator);

                    ActorPropertyAnimator::start(game_io, &self.assets, entities, *entity);
                }
            }
            ServerPacket::ActorMinimapColor { actor_id, color } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let entities = &mut self.base_scene.entities;

                    if let Ok(minimap_marker) =
                        entities.query_one_mut::<&mut PlayerMinimapMarker>(*entity)
                    {
                        minimap_marker.color = Color::from(color);
                    }
                }
            }
            ServerPacket::SynchronizeUpdates => {
                self.synchronizing_packets = true;
            }
            ServerPacket::EndSynchronization => {
                let packets = std::mem::take(&mut self.stored_packets);

                for packet in packets {
                    self.handle_packet(game_io, packet)
                }
            }
        }
    }

    fn push_server_textbox_interface(&mut self, interface: impl TextboxInterface + 'static) {
        self.base_scene
            .menu_manager
            .push_textbox_interface(interface);

        if let Some(remove) = self.doorstop_remover.take() {
            remove();
        }

        // keep the textbox open until the server receives our response or another textbox comes in
        let (doorstop_interface, remover) = TextboxDoorstop::new();
        self.doorstop_remover = Some(remover);
        self.base_scene
            .menu_manager
            .push_textbox_interface(doorstop_interface);
    }

    fn remove_actor(&mut self, actor_id: &str) {
        if let Some((_, entity)) = self.actor_id_map.remove_by_left(actor_id) {
            let _ = self.base_scene.entities.despawn(entity);
        }
    }

    fn handle_events(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.base_scene.event_receiver.try_recv() {
            match event {
                OverworldEvent::SystemMessage { message } => {
                    let interface = TextboxMessage::new(message);
                    self.base_scene
                        .menu_manager
                        .push_textbox_interface(interface);
                }
                OverworldEvent::TextboxResponse(response) => {
                    (self.send_packet)(
                        Reliability::ReliableOrdered,
                        ClientPacket::TextBoxResponse { response },
                    );
                }
                OverworldEvent::PromptResponse(response) => {
                    (self.send_packet)(
                        Reliability::ReliableOrdered,
                        ClientPacket::PromptResponse { response },
                    );
                }
                OverworldEvent::BattleStatistics(statistics) => {
                    let player_data = &self.base_scene.player_data;

                    let battle_stats = match statistics {
                        Some(statistics) => statistics,
                        None => BattleStatistics {
                            health: player_data.health,
                            emotion: player_data.emotion,
                            ran: true,
                            ..Default::default()
                        },
                    };

                    (self.send_packet)(
                        Reliability::ReliableOrdered,
                        ClientPacket::BattleResults { battle_stats },
                    );
                }
                OverworldEvent::WarpIn {
                    target_entity,
                    position,
                    direction,
                } => {
                    WarpEffect::warp_in(
                        game_io,
                        &mut self.base_scene,
                        target_entity,
                        position,
                        direction,
                        |_, _| {},
                    );
                }
                OverworldEvent::PendingWarp { entity } => {
                    let map = &mut self.base_scene.map;

                    let query_result = map
                        .object_entities_mut()
                        .query_one_mut::<&ObjectData>(entity);

                    if let Ok(data) = query_result {
                        match data.object_type {
                            ObjectType::CustomWarp | ObjectType::CustomServerWarp => {
                                (self.send_packet)(
                                    Reliability::Reliable,
                                    ClientPacket::CustomWarp {
                                        tile_object_id: data.id,
                                    },
                                );
                            }
                            _ => {
                                log::error!("Unhandled {:?}", data.object_type);
                            }
                        }
                    }
                }
                OverworldEvent::TransferServer { address, data } => {
                    let transition = crate::transitions::new_connect(game_io);
                    let scene = InitialConnectScene::new(game_io, address, data, false);
                    let next_scene = NextScene::new_swap(scene).with_transition(transition);

                    self.next_scene_queue.push_back(next_scene);
                }
                OverworldEvent::Disconnected { message } => {
                    let event_sender = self.base_scene.event_sender.clone();
                    let interface = TextboxMessage::new(message).with_callback(move || {
                        event_sender.send(OverworldEvent::Leave).unwrap();
                    });

                    self.base_scene.menu_manager.use_player_avatar(game_io);
                    self.base_scene
                        .menu_manager
                        .push_textbox_interface(interface);

                    if let Some(remove) = self.doorstop_remover.take() {
                        // we might never get a TextBoxResponseAck
                        // remove the doorstop to prevent a soft lock
                        remove();
                    }

                    self.connected = false;
                }
                OverworldEvent::Leave => {
                    let transition = crate::transitions::new_connect(game_io);
                    *self.base_scene.next_scene() =
                        NextScene::new_pop().with_transition(transition);
                }
            }
        }
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::ShoulderR) {
            self.base_scene.menu_manager.use_player_avatar(game_io);
            let event_sender = self.base_scene.event_sender.clone();

            let interface = TextboxQuestion::new(String::from("Jack out?"), move |response| {
                if response {
                    event_sender.send(OverworldEvent::Leave).unwrap();
                }
            });

            self.base_scene
                .menu_manager
                .push_textbox_interface(interface);
        }

        if input_util.was_just_pressed(Input::Confirm) {
            self.handle_interaction(0);
        }

        if input_util.was_just_pressed(Input::ShoulderL) {
            self.handle_interaction(1);
        }
    }

    fn handle_interaction(&mut self, button: u8) {
        let player_data = &self.base_scene.player_data;
        let send_packet = &self.send_packet;

        // objects have highest priority
        if let Some(tile_object_id) = player_data.object_interaction {
            send_packet(
                Reliability::Reliable,
                ClientPacket::ObjectInteraction {
                    tile_object_id,
                    button,
                },
            );
            return;
        }

        // then actors
        if let Some(actor_id) = player_data.actor_interaction.clone() {
            send_packet(
                Reliability::Reliable,
                ClientPacket::ActorInteraction { actor_id, button },
            );
            return;
        }

        // finally fallback to tile interaction
        if let Some(Vec3 { x, y, z }) = player_data.tile_interaction {
            send_packet(
                Reliability::Reliable,
                ClientPacket::TileInteraction { x, y, z, button },
            );
        }
    }

    fn send_ready_packet(&self, game_io: &GameIO) {
        (self.send_packet)(
            Reliability::ReliableOrdered,
            ClientPacket::Ready {
                time: ms_time(game_io),
            },
        );
    }

    fn send_position(&mut self, game_io: &GameIO) {
        if !self.connected {
            return;
        }

        let instant = game_io.frame_start_instant();

        if instant - self.last_position_send < SERVER_TICK_RATE {
            return;
        }

        let entities = &mut self.base_scene.entities;
        let player_entity = self.base_scene.player_data.entity;
        let (position, direction) = entities
            .query_one_mut::<(&Vec3, &Direction)>(player_entity)
            .unwrap();

        let tile_position = self.base_scene.map.world_3d_to_tile_space(*position);

        (self.send_packet)(
            Reliability::UnreliableSequenced,
            ClientPacket::Position {
                creation_time: ms_time(game_io),
                x: tile_position.x,
                y: tile_position.y,
                z: tile_position.z,
                direction: *direction,
            },
        );

        self.last_position_send = instant;
    }

    fn handle_next_scene(&mut self, game_io: &GameIO) {
        let base_scene_next_scene = self.base_scene.next_scene().take();

        if base_scene_next_scene.is_some() {
            self.next_scene_queue.push_back(base_scene_next_scene);
        }

        if game_io.is_in_transition() || !self.next_scene.is_none() {
            return;
        }

        let Some(next_scene) = self.next_scene_queue.pop_front() else {
            return;
        };

        if !matches!(&next_scene, NextScene::Push { .. }) && self.connected {
            // leaving as anything that isn't NextScene::Push will end up dropping this scene
            // notify the server
            (self.send_packet)(Reliability::ReliableOrdered, ClientPacket::Logout);
            self.connected = false;
        }

        self.next_scene = next_scene;
    }
}

impl Scene for OverworldOnlineScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        // handle events triggered from other scenes
        // should be called before handling packets, but it's not necessary to do this every frame
        self.handle_events(game_io);

        let previous_player_id = self.base_scene.player_data.package_id.clone();
        self.base_scene.enter(game_io);

        // send boosts
        self.send_boosts(game_io);

        if previous_player_id != self.base_scene.player_data.package_id {
            // send avatar data
            self.send_avatar_data(game_io);
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.handle_packets(game_io);

        system_actor_property_animation(game_io, &self.assets, &mut self.base_scene);
        system_movement_interpolation(game_io, &mut self.base_scene);
        self.base_scene.update(game_io);
        self.send_position(game_io);

        if !self.base_scene.is_input_locked(game_io) {
            self.handle_input(game_io);
        }

        self.handle_events(game_io);
        self.handle_next_scene(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        if !self.transferring {
            self.base_scene.draw(game_io, render_pass);
        }
    }
}

fn ms_time(game_io: &GameIO) -> u64 {
    let duration = game_io.frame_start_instant() - game_io.game_start_instant();

    duration.as_millis() as u64
}
