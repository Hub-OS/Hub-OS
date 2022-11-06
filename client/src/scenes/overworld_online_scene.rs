use super::{NetplayInitScene, OverworldSceneBase};
use crate::battle::BattleProps;
use crate::bindable::Emotion;
use crate::overworld::{components::*, movement_interpolation_system, CameraAction};
use crate::overworld::{Item, ServerAssetManager};
use crate::packages::PackageNamespace;
use crate::render::ui::{
    TextboxDoorstop, TextboxDoorstopRemover, TextboxInterface, TextboxMessage, TextboxPrompt,
    TextboxQuestion, TextboxQuiz,
};
use crate::render::AnimatorLoopMode;
use crate::resources::*;
use crate::scenes::BattleScene;
use crate::transitions::{ColorFadeTransition, DEFAULT_FADE_DURATION, DRAMATIC_FADE_DURATION};
use bimap::BiMap;
use framework::prelude::*;
use packets::structures::{BattleStatistics, FileHash};
use packets::{
    address_parsing, ClientAssetType, ClientPacket, Reliability, ServerPacket, SERVER_TICK_RATE,
};
use std::collections::{HashMap, VecDeque};

enum Event {
    BattleStatistics(Option<BattleStatistics>),
    ServerTextboxResponse(u8),
    ServerPromptResponse(String),
    RemoveActor { actor_id: String },
    Disconnected { message: String },
    Leave,
}

pub struct OverworldOnlineScene {
    base_scene: OverworldSceneBase,
    next_scene: NextScene<Globals>,
    next_scene_queue: VecDeque<NextScene<Globals>>,
    connected: bool,
    server_address: String,
    max_payload_size: u16,
    send_packet: ClientPacketSender,
    packet_receiver: ServerPacketReceiver,
    synchronizing_packets: bool,
    stored_packets: Vec<ServerPacket>,
    last_position_send: Instant,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    assets: ServerAssetManager,
    custom_emotes_path: String,
    actor_id_map: BiMap<String, hecs::Entity>,
    doorstop_remover: Option<TextboxDoorstopRemover>,
    encounter_packages: HashMap<String, String>, // server_path -> package_id
    loaded_zips: HashMap<String, FileHash>,      // server_path -> hash
}

impl OverworldOnlineScene {
    pub fn new(
        game_io: &mut GameIO<Globals>,
        address: String,
        max_payload_size: u16,
        send_packet: ClientPacketSender,
        packet_receiver: flume::Receiver<ServerPacket>,
    ) -> Self {
        let mut base_scene = OverworldSceneBase::new(game_io);

        let player_entity = base_scene.player_data.entity;
        let entities = &mut base_scene.entities;
        entities
            .insert_one(player_entity, HiddenSprite::default())
            .unwrap();

        let (event_sender, event_receiver) = flume::unbounded();
        let assets = ServerAssetManager::new(game_io, &address);

        Self {
            base_scene,
            next_scene: NextScene::None,
            next_scene_queue: VecDeque::new(),
            connected: true,
            server_address: address,
            // provide a min size for underflow protection
            max_payload_size: max_payload_size.max(100),
            send_packet,
            packet_receiver,
            synchronizing_packets: false,
            stored_packets: Vec::new(),
            last_position_send: game_io.frame_start_instant(),
            event_sender,
            event_receiver,
            assets,
            custom_emotes_path: ResourcePaths::BLANK.to_string(),
            actor_id_map: BiMap::new(),
            doorstop_remover: None,
            encounter_packages: HashMap::new(),
            loaded_zips: HashMap::new(),
        }
    }

    pub fn start_connection(&self, game_io: &GameIO<Globals>) {
        let globals = game_io.globals();
        let global_save = &globals.global_save;

        let send_packet = &self.send_packet;

        // send login packet
        send_packet(
            Reliability::ReliableOrdered,
            ClientPacket::Login {
                username: global_save.nickname.clone(),
                identity: String::new(),
                data: address_parsing::slice_data(&self.server_address).to_string(),
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

        // send player assets
        let player_package = global_save.player_package(game_io).unwrap();

        let assets = &globals.assets;

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
        // todo: add hp modification by ncp
        send_packet(
            Reliability::ReliableOrdered,
            ClientPacket::AvatarChange {
                name: player_package.name.clone(),
                element: player_package.element.to_string(),
                max_health: player_package.health as u32,
            },
        );

        // nothing else to send, request join
        send_packet(Reliability::ReliableOrdered, ClientPacket::RequestJoin);
    }

    pub fn packet_receiver(&self) -> &ServerPacketReceiver {
        &self.packet_receiver
    }

    pub fn handle_packets(&mut self, game_io: &mut GameIO<Globals>) {
        while let Ok(packet) = self.packet_receiver.try_recv() {
            self.handle_packet(game_io, packet);
        }

        if self.connected && self.packet_receiver.is_disconnected() {
            self.event_sender
                .send(Event::Disconnected {
                    message: String::from(
                        "Everything is still.\x01..\x01\nLooks like we've been disconnected.",
                    ),
                })
                .unwrap();
        }
    }

    pub fn handle_packet(&mut self, game_io: &mut GameIO<Globals>, packet: ServerPacket) {
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
                log::warn!("Authorize hasn't been implemented")
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

                if warp_in {
                    WarpEffect::warp_in(
                        game_io,
                        &mut self.base_scene,
                        player_entity,
                        spawn_position,
                        spawn_direction,
                        || {},
                    );
                } else {
                    let (position, direction) = entities
                        .query_one_mut::<(&mut Vec3, &mut Direction)>(player_entity)
                        .unwrap();
                    *position = spawn_position;
                    *direction = spawn_direction;

                    entities.remove_one::<HiddenSprite>(player_entity);
                }
            }
            ServerPacket::CompleteConnection => {
                self.send_ready_packet(game_io);
            }
            ServerPacket::TransferWarp => {
                let player_entity = self.base_scene.player_data.entity;
                let send_packet = self.send_packet.clone();

                WarpEffect::warp_out(game_io, &mut self.base_scene, player_entity, move || {
                    send_packet(Reliability::ReliableOrdered, ClientPacket::TransferredOut);
                });
            }
            ServerPacket::TransferStart => log::warn!("TransferStart hasn't been implemented"),
            ServerPacket::TransferComplete { warp_in, direction } => {
                let player_entity = self.base_scene.player_data.entity;
                let send_packet = self.send_packet.clone();

                let entities = &mut self.base_scene.entities;
                let (position, direction) = entities
                    .query_one_mut::<(&Vec3, &Direction)>(player_entity)
                    .unwrap();

                if warp_in {
                    let position = *position;
                    let direction = *direction;

                    WarpEffect::warp_in(
                        game_io,
                        &mut self.base_scene,
                        player_entity,
                        position,
                        direction,
                        move || {},
                    );
                }

                self.send_ready_packet(game_io);
            }
            ServerPacket::TransferServer {
                address,
                data,
                warp_out,
            } => log::warn!("TransferServer hasn't been implemented"),
            ServerPacket::Kick { reason } => {
                self.event_sender
                    .send(Event::Disconnected {
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
                        log::warn!("server sent unknown AssetType? {:?}", asset_path);
                    }
                }
            }
            ServerPacket::CustomEmotesPath { asset_path } => {
                self.custom_emotes_path = asset_path;
            }
            ServerPacket::MapUpdate { map_path } => {
                use crate::overworld::load_map;

                let data = self.assets.text(&map_path);

                if let Some(map) = load_map(game_io, &self.assets, &data) {
                    self.base_scene.set_world(game_io, &self.assets, map);
                } else {
                    log::warn!("failed to load map provided by server");
                }
            }
            ServerPacket::Health { health, max_health } => {
                self.base_scene.player_data.health = health as i32;
                self.base_scene.player_data.max_health = max_health as i32;
            }
            ServerPacket::Emotion { emotion } => {
                use num_traits::FromPrimitive;

                if let Some(emotion) = Emotion::from_u8(emotion) {
                    self.base_scene.player_data.emotion = emotion;
                }
            }
            ServerPacket::Money { money } => {
                self.base_scene.player_data.money = money;
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
                game_io.globals().audio.play_sound(&sound);
            }
            ServerPacket::ExcludeObject { id } => {
                log::warn!("ExcludeObject hasn't been implemented")
            }
            ServerPacket::IncludeObject { id } => {
                log::warn!("IncludeObject hasn't been implemented")
            }
            ServerPacket::ExcludeActor { actor_id } => {
                log::warn!("ExcludeActor hasn't been implemented")
            }
            ServerPacket::IncludeActor { actor_id } => {
                log::warn!("IncludeActor hasn't been implemented")
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
                        || {},
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

                let event_sender = self.event_sender.clone();
                let message_interface = TextboxMessage::new(message).with_callback(move || {
                    event_sender.send(Event::ServerTextboxResponse(0)).unwrap();
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

                let event_sender = self.event_sender.clone();
                let question_interface = TextboxQuestion::new(message, move |response| {
                    event_sender
                        .send(Event::ServerTextboxResponse(response as u8))
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

                let event_sender = self.event_sender.clone();
                let quiz_interface = TextboxQuiz::new(options, move |response| {
                    event_sender
                        .send(Event::ServerTextboxResponse(response as u8))
                        .unwrap();
                });

                self.push_server_textbox_interface(quiz_interface);
            }
            ServerPacket::Prompt {
                character_limit,
                default_text,
            } => {
                let event_sender = self.event_sender.clone();
                let prompt_interface = TextboxPrompt::new(move |response| {
                    event_sender
                        .send(Event::ServerPromptResponse(response))
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

                let globals = game_io.globals();
                let assets = &globals.assets;

                let hash = match self.loaded_zips.get(&package_path) {
                    Some(hash) => *hash,
                    None => {
                        let bytes = self.assets.binary(&package_path);
                        let hash = FileHash::hash(&bytes);
                        assets.load_virtual_zip(game_io, hash, bytes);
                        hash
                    }
                };

                let globals = game_io.globals_mut();

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
            ServerPacket::InitiateEncounter { package_path, data } => {
                let globals = game_io.globals();

                if let Some(package_id) = self.encounter_packages.get(&package_path) {
                    // get package
                    let battle_package = globals
                        .battle_packages
                        .package_or_fallback(PackageNamespace::Server, package_id);

                    let mut props = BattleProps::new_with_defaults(game_io, battle_package);

                    // callback
                    let event_sender = self.event_sender.clone();
                    props.statistics_callback = Some(Box::new(move |statistics| {
                        let _ = event_sender.send(Event::BattleStatistics(statistics));
                    }));

                    // copy background
                    props.background = self
                        .base_scene
                        .map
                        .background_properties()
                        .generate_background(game_io, &self.assets);

                    // create scene
                    let scene = BattleScene::new(game_io, props);
                    let transition =
                        ColorFadeTransition::new(game_io, Color::WHITE, DRAMATIC_FADE_DURATION);

                    let next_scene = NextScene::new_push(scene).with_transition(transition);
                    self.next_scene_queue.push_back(next_scene);
                }
            }
            ServerPacket::InitiateNetplay {
                package_path,
                data,
                remote_players,
            } => {
                (self.send_packet)(Reliability::ReliableOrdered, ClientPacket::EncounterStart);

                let background = self
                    .base_scene
                    .map
                    .background_properties()
                    .generate_background(game_io, &self.assets);

                let event_sender = self.event_sender.clone();
                let statistics_callback = Box::new(move |statistics| {
                    let _ = event_sender.send(Event::BattleStatistics(statistics));
                });

                let scene = NetplayInitScene::new(
                    game_io,
                    Some(background),
                    package_path.map(|path| (PackageNamespace::Server, path)),
                    data,
                    remote_players,
                    self.server_address.clone(),
                    Some(statistics_callback),
                );

                let transition =
                    ColorFadeTransition::new(game_io, Color::WHITE, DRAMATIC_FADE_DURATION);

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

                    self.base_scene
                        .spawn_player_actor(game_io, texture, animator, position)
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
                        movement_animator.enabled = false;
                    }

                    if warp_in {
                        // create warp effect
                        WarpEffect::warp_in(
                            game_io,
                            &mut self.base_scene,
                            entity,
                            position,
                            initial_direction,
                            || {},
                        );
                    }

                    self.actor_id_map.insert(actor_id, entity);
                }
            }
            ServerPacket::ActorDisconnected { actor_id, warp_out } => {
                if warp_out {
                    if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                        let event_sender = self.event_sender.clone();

                        WarpEffect::warp_out(game_io, &mut self.base_scene, *entity, move || {
                            event_sender.send(Event::RemoveActor { actor_id }).unwrap();
                        });
                    }
                } else {
                    self.event_sender
                        .send(Event::RemoveActor { actor_id })
                        .unwrap();
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

                    if let Ok(interpolator) =
                        entities.query_one_mut::<(&mut MovementInterpolator)>(*entity)
                    {
                        let tile_position = Vec3::new(x, y, z);
                        let position = self.base_scene.map.tile_3d_to_world(tile_position);

                        if interpolator.is_movement_impossible(position) {
                            // todo: warp
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

                    movement_animator.enabled = false;
                }
            }
            ServerPacket::ActorPropertyKeyFrames {
                actor_id,
                keyframes,
            } => log::warn!("ActorPropertyKeyFrames hasn't been implemented"),
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

    fn handle_events(&mut self, game_io: &GameIO<Globals>) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::BattleStatistics(statistics) => {
                    let player_data = &self.base_scene.player_data;

                    let battle_stats = match statistics {
                        Some(statistics) => statistics,
                        None => BattleStatistics {
                            health: player_data.health as u32,
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
                Event::ServerTextboxResponse(response) => (self.send_packet)(
                    Reliability::ReliableOrdered,
                    ClientPacket::TextBoxResponse { response },
                ),
                Event::ServerPromptResponse(response) => (self.send_packet)(
                    Reliability::ReliableOrdered,
                    ClientPacket::PromptResponse { response },
                ),
                Event::RemoveActor { actor_id } => {
                    if let Some((_, entity)) = self.actor_id_map.remove_by_left(&actor_id) {
                        let _ = self.base_scene.entities.despawn(entity);
                    }
                }
                Event::Disconnected { message } => {
                    let event_sender = self.event_sender.clone();
                    let interface = TextboxMessage::new(message).with_callback(move || {
                        event_sender.send(Event::Leave).unwrap();
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
                Event::Leave => {
                    *self.base_scene.next_scene() = NextScene::new_pop().with_transition(
                        ColorFadeTransition::new(game_io, Color::WHITE, DEFAULT_FADE_DURATION),
                    );
                }
            }
        }
    }

    fn handle_input(&mut self, game_io: &mut GameIO<Globals>) {
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::ShoulderR) {
            self.base_scene.menu_manager.use_player_avatar(game_io);
            let event_sender = self.event_sender.clone();
            let send_packet = self.send_packet.clone();

            let interface = TextboxQuestion::new(String::from("Jack out?"), move |response| {
                if response {
                    send_packet(Reliability::ReliableOrdered, ClientPacket::Logout);
                    event_sender.send(Event::Leave).unwrap();
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

    fn send_ready_packet(&self, game_io: &GameIO<Globals>) {
        (self.send_packet)(
            Reliability::ReliableOrdered,
            ClientPacket::Ready {
                time: ms_time(game_io),
            },
        );
    }

    fn send_position(&mut self, game_io: &GameIO<Globals>) {
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

    fn handle_next_scene(&mut self, game_io: &GameIO<Globals>) {
        let base_scene_next_scene = self.base_scene.next_scene().take();

        if base_scene_next_scene.is_some() {
            self.next_scene_queue.push_back(base_scene_next_scene)
        }

        if !game_io.is_in_transition() && self.next_scene.is_none() {
            if let Some(next_scene) = self.next_scene_queue.pop_front() {
                self.next_scene = next_scene;
            }
        }
    }
}

impl Scene<Globals> for OverworldOnlineScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO<Globals>) {
        // handle events triggered from other scenes
        // should be called before handling packets, but it's not necessary to do this every frame
        self.handle_events(game_io);
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        self.handle_packets(game_io);

        self.base_scene.update(game_io);
        self.send_position(game_io);

        if !self.base_scene.is_input_locked(game_io) {
            self.handle_input(game_io);
        }

        movement_interpolation_system(game_io, &mut self.base_scene);
        self.handle_events(game_io);
        self.handle_next_scene(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        self.base_scene.draw(game_io, render_pass)
    }
}

fn ms_time(game_io: &GameIO<Globals>) -> u64 {
    let duration = game_io.frame_start_instant() - game_io.game_start_instant();

    duration.as_millis() as u64
}
