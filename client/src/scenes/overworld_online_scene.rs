use super::{
    InitialConnectScene, NetplayInitScene, NetplayProps, PackageScene, ServerEditProp,
    ServerEditScene,
};
use crate::battle::BattleProps;
use crate::bindable::SpriteColorMode;
use crate::overworld::components::*;
use crate::overworld::*;
use crate::packages::{PackageId, PackageNamespace};
use crate::render::ui::{
    PackageListing, Text, TextStyle, TextboxDoorstop, TextboxDoorstopKey, TextboxInterface,
    TextboxMessage, TextboxMessageAuto, TextboxPrompt, TextboxQuestion, TextboxQuiz,
};
use crate::render::{AnimatorLoopMode, SpriteColorQueue};
use crate::resources::*;
use crate::saves::{BlockGrid, Card};
use crate::scenes::BattleInitScene;
use bimap::BiMap;
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use packets::structures::{
    ActorId, ActorProperty, BattleId, BattleStatistics, FileHash, PackageCategory,
    SpriteDefinition, SpriteId, TextboxOptions,
};
use packets::{
    ClientAssetType, ClientPacket, Reliability, SERVER_TICK_RATE, ServerPacket, address_parsing,
};
use std::collections::{HashMap, VecDeque};
use strum::IntoEnumIterator;

pub struct OverworldOnlineScene {
    area: OverworldArea,
    menu_manager: OverworldMenuManager,
    hud: OverworldHud,
    auto_emotes: AutoEmotes,
    next_scene: NextScene,
    next_scene_queue: VecDeque<(AutoEmote, NextScene)>,
    connected: bool,
    transferring: bool,
    identity: Identity,
    server_address: String,
    send_packet: ClientPacketSender,
    packet_receiver: ServerPacketReceiver,
    battle_sender: flume::Sender<(BattleId, String)>,
    battle_receiver: flume::Receiver<(BattleId, String)>,
    synchronizing_packets: bool,
    stored_packets: Vec<ServerPacket>,
    previous_boost_packet: Option<ClientPacket>,
    last_position_send: Instant,
    assets: ServerAssetManager,
    actor_id_map: BiMap<ActorId, hecs::Entity>,
    sprite_id_map: HashMap<SpriteId, hecs::Entity>,
    excluded_actors: HashMap<ActorId, usize>,
    excluded_objects: HashMap<u32, usize>,
    doorstop_key: Option<TextboxDoorstopKey>,
    encounter_packages: HashMap<String, PackageId>, // server_path -> package_id
    loaded_zips: HashMap<String, FileHash>,         // server_path -> hash
}

impl OverworldOnlineScene {
    pub fn new(
        game_io: &GameIO,
        address: String,
        send_packet: ClientPacketSender,
        packet_receiver: flume::Receiver<ServerPacket>,
    ) -> Self {
        let mut area = OverworldArea::new(game_io);

        let player_entity = area.player_data.entity;
        let entities = &mut area.entities;
        entities
            .insert_one(player_entity, Excluded::default())
            .unwrap();

        let assets = ServerAssetManager::new(game_io, &address);

        // menus
        let mut menu_manager = OverworldMenuManager::new(game_io);
        menu_manager.update_player_data(&area.player_data);

        // map menu
        let map_menu = MapMenu::new(game_io);
        let map_menu_index = menu_manager.register_menu(Box::new(map_menu));
        menu_manager.bind_menu(Input::Map, map_menu_index);

        // emote menu
        let emote_menu = EmoteMenu::new(game_io);
        let emote_menu_index = menu_manager.register_menu(Box::new(emote_menu));
        menu_manager.bind_menu(Input::Option2, emote_menu_index);

        // hud
        let hud = OverworldHud::new(game_io, area.player_data.health);

        // auto emotes
        let auto_emotes = AutoEmotes::new(&area.emote_animator);

        let (battle_sender, battle_receiver) = flume::unbounded();

        Self {
            area,
            menu_manager,
            hud,
            auto_emotes,
            next_scene: NextScene::None,
            next_scene_queue: VecDeque::new(),
            connected: true,
            transferring: false,
            identity: Identity::for_address(&address),
            server_address: address,
            send_packet,
            packet_receiver,
            battle_sender,
            battle_receiver,
            synchronizing_packets: false,
            stored_packets: Vec::new(),
            previous_boost_packet: None,
            last_position_send: game_io.frame_start_instant(),
            assets,
            actor_id_map: BiMap::new(),
            sprite_id_map: HashMap::new(),
            excluded_actors: Default::default(),
            excluded_objects: Default::default(),
            doorstop_key: None,
            encounter_packages: HashMap::new(),
            loaded_zips: HashMap::new(),
        }
    }

    pub fn start_connection(&mut self, game_io: &GameIO, data: Option<String>) {
        let globals = Globals::from_resources(game_io);
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

        // send boosts before sending avatar data
        // allows the server to accurately calculate health
        self.send_boosts(game_io);

        // send avatar data
        self.send_avatar_data(game_io);
    }

    pub fn sync_assets(
        &mut self,
        game_io: &mut GameIO,
        package_list: Vec<(String, PackageCategory, PackageId, FileHash)>,
    ) {
        let globals = Globals::from_resources(game_io);
        let namespace = PackageNamespace::Local;

        for (remote_path, category, package_id, file_hash) in package_list {
            if self.assets.file_hash(&remote_path) == Some(file_hash) {
                // already up to date, skip
                continue;
            }

            let Some(package_info) = globals.package_info(category, namespace, &package_id) else {
                // don't have this package
                continue;
            };

            if package_info.hash != file_hash {
                // our package doesn't match
                continue;
            }

            let zip_path = format!("{}{}.zip", ResourcePaths::mod_cache_folder(), file_hash);
            let Ok(data) = std::fs::read(&zip_path) else {
                continue;
            };

            self.assets.store_asset(remote_path, file_hash, data, true);
        }

        // send asset found signals
        for asset in self.assets.stored_assets() {
            (self.send_packet)(
                Reliability::ReliableOrdered,
                ClientPacket::AssetFound {
                    path: asset.remote_path,
                    hash: asset.hash,
                },
            );
        }
    }

    pub fn request_join(&mut self) {
        let send_packet = &self.send_packet;
        send_packet(Reliability::ReliableOrdered, ClientPacket::RequestJoin);
    }

    pub fn send_preferences(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);

        let send_packet = &self.send_packet;
        send_packet(
            Reliability::ReliableOrdered,
            ClientPacket::NetplayPreferences {
                force_relay: globals.config.force_relay,
            },
        );
    }

    pub fn send_boosts(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);
        let global_save = &globals.global_save;

        let blocks = global_save.active_blocks().to_vec();
        let block_grid = BlockGrid::new(PackageNamespace::Local).with_blocks(game_io, blocks);

        let packet = ClientPacket::Boost {
            health_boost: self.area.player_data.health_boost,
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
        let globals = Globals::from_resources(game_io);
        let global_save = &globals.global_save;
        let player_package = global_save.player_package(game_io).unwrap();

        let assets = &globals.assets;
        let send_packet = &self.send_packet;

        vec![
            ClientPacket::Asset {
                asset_type: ClientAssetType::Texture,
                data: assets.binary(&player_package.overworld_paths.texture),
            },
            ClientPacket::Asset {
                asset_type: ClientAssetType::Animation,
                data: assets.binary(&player_package.overworld_paths.animation),
            },
            ClientPacket::Asset {
                asset_type: ClientAssetType::MugshotTexture,
                data: assets.binary(&player_package.mugshot_paths.texture),
            },
            ClientPacket::Asset {
                asset_type: ClientAssetType::MugshotAnimation,
                data: assets.binary(&player_package.mugshot_paths.animation),
            },
        ]
        .into_iter()
        .for_each(|packet| send_packet(Reliability::ReliableOrdered, packet));

        // send avatar change signal
        send_packet(
            Reliability::ReliableOrdered,
            ClientPacket::AvatarChange {
                name: player_package.name.to_string(),
                element: player_package.element.to_string(),
                base_health: player_package.health,
            },
        );
    }

    pub fn packet_receiver(&self) -> &ServerPacketReceiver {
        &self.packet_receiver
    }

    fn update_ui(&mut self, game_io: &mut GameIO) {
        self.hud.update(&self.area);

        let next_scene = self.menu_manager.update(game_io, &mut self.area);

        if self.next_scene.is_none() && next_scene.is_some() {
            self.auto_emotes.set_auto_emote(AutoEmote::Menu);
            self.next_scene = next_scene;
        }
    }

    pub fn handle_packets(&mut self, game_io: &mut GameIO) {
        if !self.connected {
            return;
        }

        while let Ok(packet) = self.packet_receiver.try_recv() {
            self.handle_packet(game_io, packet);
        }

        if self.packet_receiver.is_disconnected() {
            let globals = Globals::from_resources(game_io);

            self.area
                .event_sender
                .send(OverworldEvent::Disconnected {
                    message: globals.translate("server-disconnected"),
                })
                .unwrap();
        }
    }

    pub fn handle_packet(&mut self, game_io: &mut GameIO, packet: ServerPacket) {
        if self.synchronizing_packets && packet != ServerPacket::EndSynchronization {
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
            ServerPacket::PackageList { packages } => {
                // handled in initial_connect_scene
            }
            ServerPacket::Authorize { address, data } => {
                let globals = Globals::from_resources_mut(game_io);
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
                let player_entity = self.area.player_data.entity;
                self.actor_id_map.insert(actor_id, player_entity);

                let entities = &mut self.area.entities;

                let map = &self.area.map;
                let tile_position = Vec3::new(spawn_x, spawn_y, spawn_z);
                let spawn_position = map.tile_3d_to_world(tile_position);

                let (position, direction) = entities
                    .query_one_mut::<(&mut Vec3, &mut Direction)>(player_entity)
                    .unwrap();
                *position = spawn_position;

                if !spawn_direction.is_none() {
                    *direction = spawn_direction;
                }

                entities.remove_one::<Excluded>(player_entity);

                if warp_in {
                    WarpEffect::warp_in(
                        game_io,
                        &mut self.area,
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
                let player_entity = self.area.player_data.entity;
                let send_packet = self.send_packet.clone();

                WarpEffect::warp_out(game_io, &mut self.area, player_entity, move |_, _| {
                    send_packet(Reliability::ReliableOrdered, ClientPacket::TransferredOut);
                });
            }
            ServerPacket::TransferStart => {
                self.excluded_actors.clear();
                self.excluded_objects.clear();
                self.transferring = true;

                // despawn all other actors
                let player_entity = self.area.player_data.entity;
                let (player_id, _) = self.actor_id_map.remove_by_right(&player_entity).unwrap();

                for &entity in self.actor_id_map.right_values() {
                    let _ = self.area.entities.despawn(entity);
                    self.area.despawn_sprite_attachments(entity);
                }

                self.actor_id_map.clear();
                self.actor_id_map.insert(player_id, player_entity);
            }
            ServerPacket::TransferComplete { warp_in, direction } => {
                let player_entity = self.area.player_data.entity;
                let send_packet = self.send_packet.clone();

                let entities = &mut self.area.entities;
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
                        &mut self.area,
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
                let player_entity = self.area.player_data.entity;
                let data = if data.is_empty() { None } else { Some(data) };

                WarpEffect::warp_out(game_io, &mut self.area, player_entity, |_, scene| {
                    let event = OverworldEvent::TransferServer { address, data };
                    scene.event_sender.send(event).unwrap();
                });
            }
            ServerPacket::Kick { reason } => {
                let globals = Globals::from_resources(game_io);
                self.area
                    .event_sender
                    .send(OverworldEvent::Disconnected {
                        message: globals
                            .translate_with_args("server-kicked", vec![("reason", reason.into())]),
                    })
                    .unwrap();
            }
            ServerPacket::RemoveAsset { path } => self.assets.delete_asset(&path),
            ServerPacket::AssetStreamStart {
                name,
                hash,
                cache_to_disk,
                data_type,
                size,
            } => {
                self.loaded_zips.remove(&name);
                self.assets
                    .start_download(name, hash, size as usize, data_type, cache_to_disk);
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
                        self.assets.audio(game_io, &asset_path);
                    }
                    AssetDataType::Unknown => {
                        log::warn!("Server sent unknown AssetType? {asset_path:?}");
                    }
                }
            }
            ServerPacket::CustomEmotesPath {
                animation_path,
                texture_path,
            } => {
                self.area.emote_animator = Animator::load_new(&self.assets, &animation_path);
                self.area.emote_sprite = self.assets.new_sprite(game_io, &texture_path);
                self.auto_emotes.read_animator(&self.area.emote_animator);
            }
            ServerPacket::MapUpdate { map_path } => {
                use crate::overworld::load_map;

                let data = self.assets.text(&map_path);

                if let Some(mut map) = load_map(game_io, &self.assets, &data) {
                    for id in self.excluded_objects.keys() {
                        let Some(entity) = map.get_object_entity(*id) else {
                            continue;
                        };

                        Excluded::increment(map.object_entities_mut(), entity);
                    }

                    self.area.set_map(game_io, &self.assets, map);
                } else {
                    log::warn!("Failed to load map provided by server");
                }
            }
            ServerPacket::Health { health } => {
                let player_data = &mut self.area.player_data;
                player_data.health = health;

                self.menu_manager.update_player_data(player_data);
            }
            ServerPacket::BaseHealth { base_health } => {
                let player_data = &mut self.area.player_data;
                player_data.base_health = base_health;

                self.menu_manager.update_player_data(player_data);
            }
            ServerPacket::Emotion { emotion } => {
                self.area.player_data.emotion = emotion;
            }
            ServerPacket::Money { money } => {
                let player_data = &mut self.area.player_data;
                player_data.money = money;

                self.menu_manager.update_player_data(player_data);
            }
            ServerPacket::RegisterItem {
                id,
                item_definition,
            } => {
                self.area.item_registry.insert(id, item_definition);
                self.area
                    .item_registry
                    .sort_by(|_, a, _, b| a.sort_key.cmp(&b.sort_key));
            }
            ServerPacket::AddItem { id, count } => {
                self.area.player_data.inventory.give_item(&id, count);
            }
            ServerPacket::AddCard {
                package_id,
                code,
                count,
            } => {
                let card = Card { package_id, code };

                let globals = Globals::from_resources_mut(game_io);
                globals.restrictions.add_card(card, count);
            }
            ServerPacket::AddBlock {
                package_id,
                color,
                count,
            } => {
                let globals = Globals::from_resources_mut(game_io);
                globals.restrictions.add_block(package_id, color, count);
            }
            ServerPacket::EnablePlayableCharacter {
                package_id,
                enabled,
            } => {
                let globals = Globals::from_resources_mut(game_io);
                let restrictions = &mut globals.restrictions;
                restrictions.enable_player_character(package_id, enabled);
            }
            ServerPacket::PlaySound { path } => {
                if self.area.visible {
                    let sound = self.assets.audio(game_io, &path);
                    let globals = Globals::from_resources(game_io);
                    globals.audio.play_sound(&sound);
                }
            }
            ServerPacket::ExcludeObject { id } => {
                let exclude_count = self.excluded_objects.entry(id).or_default();
                *exclude_count += 1;

                let map = &mut self.area.map;

                if *exclude_count == 1
                    && let Some(entity) = map.get_object_entity(id)
                {
                    Excluded::increment(map.object_entities_mut(), entity);
                }
            }
            ServerPacket::IncludeObject { id } => {
                let exclude_count = self.excluded_objects.entry(id).or_insert(1);
                *exclude_count -= 1;

                if *exclude_count == 0 {
                    let map = &mut self.area.map;

                    if let Some(entity) = map.get_object_entity(id) {
                        Excluded::decrement(map.object_entities_mut(), entity);
                    }

                    self.excluded_objects.remove(&id);
                }
            }
            ServerPacket::ExcludeActor { actor_id } => {
                let exclude_count = self.excluded_actors.entry(actor_id).or_default();
                *exclude_count += 1;

                if *exclude_count == 1
                    && let Some(entity) = self.actor_id_map.get_by_left(&actor_id)
                {
                    let entities = &mut self.area.entities;
                    Excluded::increment(entities, *entity);
                }
            }
            ServerPacket::IncludeActor { actor_id } => {
                let exclude_count = self.excluded_actors.entry(actor_id).or_insert(1);
                *exclude_count -= 1;

                if *exclude_count == 0 {
                    if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                        let entities = &mut self.area.entities;
                        Excluded::decrement(entities, *entity);
                    }

                    self.excluded_actors.remove(&actor_id);
                }
            }
            ServerPacket::MoveCamera {
                x,
                y,
                z,
                hold_duration,
            } => {
                let world_position = self.area.map.tile_3d_to_world(Vec3 { x, y, z });
                let target = self.area.map.world_3d_to_screen(world_position);

                self.area.queue_camera_action(CameraAction::Snap {
                    target,
                    hold_duration,
                });
            }
            ServerPacket::SlideCamera { x, y, z, duration } => {
                let world_position = self.area.map.tile_3d_to_world(Vec3 { x, y, z });
                let target = self.area.map.world_3d_to_screen(world_position);

                self.area
                    .queue_camera_action(CameraAction::Slide { target, duration });
            }
            ServerPacket::ShakeCamera { strength, duration } => {
                self.area
                    .queue_camera_action(CameraAction::Shake { strength, duration });
            }
            ServerPacket::FadeCamera { color, duration } => {
                self.area.queue_camera_action(CameraAction::Fade {
                    color: color.into(),
                    duration,
                });
            }
            ServerPacket::TrackWithCamera { actor_id } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id).cloned() {
                    self.area
                        .queue_camera_action(CameraAction::TrackEntity { entity });
                }
            }
            ServerPacket::EnableCameraControls { dist_x, dist_y } => {
                log::warn!("EnableCameraControls hasn't been implemented")
            }
            ServerPacket::UnlockCamera => {
                self.area.queue_camera_action(CameraAction::Unlock);
            }
            ServerPacket::LockInput => {
                self.area.add_input_lock();
            }
            ServerPacket::UnlockInput => {
                self.area.remove_input_lock();
            }
            ServerPacket::Teleport {
                warp,
                x,
                y,
                z,
                direction,
            } => {
                let tile_position = Vec3 { x, y, z };
                let position = self.area.map.tile_3d_to_world(tile_position);

                let player_entity = self.area.player_data.entity;

                if warp {
                    WarpEffect::warp_full(
                        game_io,
                        &mut self.area,
                        player_entity,
                        position,
                        direction,
                        |_, _| {},
                    );
                } else {
                    let entities = &mut self.area.entities;
                    let (set_position, set_direction) = entities
                        .query_one_mut::<(&mut Vec3, &mut Direction)>(player_entity)
                        .unwrap();

                    *set_position = position;
                    *set_direction = direction;
                }
            }
            ServerPacket::HideHud => {
                self.hud.set_visible(false);
            }
            ServerPacket::ShowHud => {
                self.hud.set_visible(true);
            }
            ServerPacket::Message {
                message,
                textbox_options,
            } => {
                let event_sender = self.area.event_sender.clone();
                let interface = TextboxMessage::new(message).with_callback(move || {
                    event_sender
                        .send(OverworldEvent::TextboxResponse(0))
                        .unwrap();
                });

                self.push_textbox_interface_with_options(game_io, interface, textbox_options);
                self.set_textbox_doorstop();
            }
            ServerPacket::AutoMessage {
                message,
                close_delay,
                textbox_options,
            } => {
                let event_sender = self.area.event_sender.clone();
                let interface = TextboxMessageAuto::new(message)
                    .with_close_delay((close_delay * 60.0).round() as _)
                    .with_callback(move || {
                        event_sender
                            .send(OverworldEvent::TextboxResponse(0))
                            .unwrap();
                    });

                self.push_textbox_interface_with_options(game_io, interface, textbox_options);
                self.set_textbox_doorstop();
            }
            ServerPacket::Question {
                message,
                textbox_options,
            } => {
                let event_sender = self.area.event_sender.clone();
                let interface = TextboxQuestion::new(game_io, message, move |response| {
                    event_sender
                        .send(OverworldEvent::TextboxResponse(response as u8))
                        .unwrap();
                });

                self.push_textbox_interface_with_options(game_io, interface, textbox_options);
                self.set_textbox_doorstop();
            }
            ServerPacket::Quiz {
                option_a,
                option_b,
                option_c,
                textbox_options,
            } => {
                let options: &[&str; 3] = &[&option_a, &option_b, &option_c];

                let event_sender = self.area.event_sender.clone();
                let interface = TextboxQuiz::new(options, move |response| {
                    event_sender
                        .send(OverworldEvent::TextboxResponse(response as u8))
                        .unwrap();
                });

                self.push_textbox_interface_with_options(game_io, interface, textbox_options);
                self.set_textbox_doorstop();
            }
            ServerPacket::Prompt {
                character_limit,
                default_text,
            } => {
                let event_sender = self.area.event_sender.clone();
                let prompt_interface = TextboxPrompt::new(move |response| {
                    event_sender
                        .send(OverworldEvent::PromptResponse(response))
                        .unwrap();
                });

                self.menu_manager.push_textbox_interface(prompt_interface);
                self.set_textbox_doorstop();
            }
            ServerPacket::TextBoxResponseAck => {
                if self.menu_manager.remaining_textbox_interfaces() == 1 {
                    // big assumption here
                    // the last interface should be from the server
                    // todo: replace with a counter?
                    self.doorstop_key.take();
                }
            }
            ServerPacket::OpenBoard {
                topic,
                color,
                posts,
                open_instantly,
            } => {
                let send_packet = self.send_packet.clone();

                let on_select = move |id: &str| {
                    send_packet(
                        Reliability::ReliableOrdered,
                        ClientPacket::PostSelection {
                            post_id: id.to_string(),
                        },
                    );
                };

                let send_packet = self.send_packet.clone();
                let request_more = move || {
                    send_packet(Reliability::ReliableOrdered, ClientPacket::PostRequest);
                };

                let send_packet = self.send_packet.clone();
                let on_close = move || {
                    send_packet(Reliability::ReliableOrdered, ClientPacket::BoardClose);
                };

                self.menu_manager.open_bbs(
                    game_io,
                    topic,
                    color.into(),
                    !open_instantly,
                    on_select,
                    request_more,
                    on_close,
                );

                if let Some(bbs) = self.menu_manager.bbs_mut() {
                    bbs.append_posts(None, posts);
                }

                // let the server know post selections from now on are for this board
                (self.send_packet)(Reliability::ReliableOrdered, ClientPacket::BoardOpen);
            }
            ServerPacket::PrependPosts { reference, posts } => {
                if let Some(bbs) = self.menu_manager.bbs_mut() {
                    bbs.prepend_posts(reference.as_deref(), posts);
                }
            }
            ServerPacket::AppendPosts { reference, posts } => {
                if let Some(bbs) = self.menu_manager.bbs_mut() {
                    bbs.append_posts(reference.as_deref(), posts);
                }
            }
            ServerPacket::RemovePost { id } => {
                if let Some(bbs) = self.menu_manager.bbs_mut() {
                    bbs.remove_post(&id);
                }
            }
            ServerPacket::SelectionAck => {
                self.menu_manager.acknowledge_selection();
            }
            ServerPacket::CloseBoard => {
                if let Some(bbs) = self.menu_manager.bbs_mut() {
                    bbs.close();
                }
            }
            ServerPacket::OpenShop { textbox_options } => {
                let send_packet = self.send_packet.clone();

                // let the server know shop selections from now on are for this shop
                send_packet(Reliability::ReliableOrdered, ClientPacket::ShopOpen);

                let on_select = {
                    let send_packet = self.send_packet.clone();

                    move |id: &str| {
                        send_packet(
                            Reliability::ReliableOrdered,
                            ClientPacket::ShopPurchase {
                                item_id: id.to_string(),
                            },
                        );
                    }
                };

                let on_description_request = {
                    let send_packet = self.send_packet.clone();

                    move |id: &str| {
                        send_packet(
                            Reliability::ReliableOrdered,
                            ClientPacket::ShopDescriptionRequest {
                                item_id: id.to_string(),
                            },
                        );
                    }
                };

                let on_leave = {
                    let send_packet = self.send_packet.clone();

                    move || {
                        send_packet(Reliability::ReliableOrdered, ClientPacket::ShopLeave);
                    }
                };

                let on_close = {
                    let send_packet = self.send_packet.clone();

                    move || {
                        send_packet(Reliability::ReliableOrdered, ClientPacket::ShopClose);
                    }
                };

                self.menu_manager.open_shop(
                    game_io,
                    on_select,
                    on_description_request,
                    on_leave,
                    on_close,
                );

                let shop = self.menu_manager.shop_mut().unwrap();

                // todo: apply all relevant textbox options, missing font as of writing
                shop.set_shop_avatar(game_io, &self.assets, textbox_options.mug.as_ref());
                shop.set_money(self.area.player_data.money);
            }
            ServerPacket::PrependShopItems { reference, items } => {
                if let Some(shop) = self.menu_manager.shop_mut() {
                    shop.prepend_items(reference.as_deref(), items);
                }
            }
            ServerPacket::AppendShopItems { reference, items } => {
                if let Some(shop) = self.menu_manager.shop_mut() {
                    shop.append_items(reference.as_deref(), items);
                }
            }
            ServerPacket::ShopMessage { message } => {
                if let Some(shop) = self.menu_manager.shop_mut() {
                    shop.set_message(message);
                }
            }
            ServerPacket::UpdateShopItem { item } => {
                if let Some(shop) = self.menu_manager.shop_mut() {
                    shop.update_item(item);
                }
            }
            ServerPacket::RemoveShopItem { id } => {
                if let Some(shop) = self.menu_manager.shop_mut() {
                    shop.remove_item(&id);
                }
            }
            ServerPacket::ReferLink { address } => {
                if !address.starts_with("http://") && !address.starts_with("https://") {
                    log::error!("Received non http / https address: {address:?}");
                } else {
                    let message = String::from("Open link in browser?");
                    let interface = TextboxQuestion::new(game_io, message, move |yes| {
                        if yes && let Err(err) = webbrowser::open(&address) {
                            log::error!("{err:?}");
                        }
                    });

                    self.menu_manager.use_player_avatar(game_io);
                    self.menu_manager.push_textbox_interface(interface);

                    self.doorstop_key.take();
                }
            }
            ServerPacket::ReferServer { name, address } => {
                let globals = Globals::from_resources(game_io);
                let index = globals.global_save.server_list.len() + 1;
                let name = Some(name);
                let address = Some(address);

                let scene = ServerEditScene::new(
                    game_io,
                    ServerEditProp::Insert {
                        index,
                        name,
                        address,
                    },
                );

                let transition = crate::transitions::new_sub_scene(game_io);
                let next_scene = NextScene::new_push(scene).with_transition(transition);
                self.next_scene_queue
                    .push_back((AutoEmote::Menu, next_scene));
            }
            ServerPacket::ReferPackage {
                package_id,
                options,
            } => {
                let globals = Globals::from_resources(game_io);

                let ignore = options.unless_installed
                    && PackageCategory::iter().any(|category| {
                        globals
                            .package_info(category, PackageNamespace::Local, &package_id)
                            .is_some()
                    });

                if !ignore {
                    let repo = &globals.config.package_repo;
                    let encoded_id = uri_encode(package_id.as_str());
                    let uri = format!("{repo}/api/mods/{encoded_id}/meta");

                    let event_sender = self.area.event_sender.clone();

                    game_io
                        .spawn_local_task(async move {
                            let Some(value) = crate::http::request_json(&uri).await else {
                                return;
                            };

                            let listing = PackageListing::from(&value);
                            let _ = event_sender.send(OverworldEvent::PackageReferred(listing));
                        })
                        .detach()
                }
            }
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

                if !self.loaded_zips.contains_key(&package_path) {
                    let globals = Globals::from_resources(game_io);

                    let bytes = self.assets.binary(&package_path);
                    let hash = FileHash::hash(&bytes);

                    globals.assets.load_virtual_zip(game_io, hash, bytes);
                    self.loaded_zips.insert(package_path.clone(), hash);

                    let globals = Globals::from_resources_mut(game_io);

                    if let Some(package_info) =
                        globals.load_virtual_package(category, namespace, hash)
                    {
                        // save package id for starting encounters
                        self.encounter_packages
                            .insert(package_path, package_info.id.clone());
                    }
                };
            }
            ServerPacket::Restrictions { restrictions_path } => {
                let globals = Globals::from_resources_mut(game_io);
                let restrictions = &mut globals.restrictions;

                let restrictions_text = restrictions_path
                    .map(|path| self.assets.text(&path))
                    .unwrap_or_default();

                restrictions.load_restrictions_toml(restrictions_text);
            }
            ServerPacket::InitiateEncounter {
                battle_id,
                package_path,
                data,
            } => {
                let globals = Globals::from_resources(game_io);

                if let Some(package_id) = self.encounter_packages.get(&package_path) {
                    let encounter_package = Some((PackageNamespace::Server, package_id.clone()));
                    let mut props = BattleProps::new_with_defaults(game_io, encounter_package);
                    props.meta.data = data;
                    props.comms.remote_id = battle_id;
                    props.comms.server =
                        Some((self.send_packet.clone(), self.battle_receiver.clone()));

                    let player_setup = &mut props.meta.player_setups[0];
                    let player_data = &self.area.player_data;
                    player_setup.health = player_data.health;
                    player_setup.base_health = player_data.base_health;
                    player_setup.emotion = player_data.emotion.clone();

                    // callback
                    let event_sender = self.area.event_sender.clone();
                    props.statistics_callback = Some(Box::new(move |statistics| {
                        let _ = event_sender.send(OverworldEvent::BattleStatistics(statistics));
                    }));

                    // copy background
                    let bg_props = self.area.map.background_properties();
                    props.meta.background = bg_props.generate_background(game_io, &self.assets);

                    // create scene
                    let scene = BattleInitScene::new(game_io, props);

                    let transition = crate::transitions::new_battle_init(game_io);
                    let next_scene = NextScene::new_push(scene).with_transition(transition);
                    self.next_scene_queue
                        .push_back((AutoEmote::Battle, next_scene));
                } else {
                    log::error!("Failed to initiate encounter: no package for {package_path:?}")
                }
            }
            ServerPacket::InitiateNetplay {
                battle_id,
                package_path,
                data,
                seed,
                remote_players,
            } => {
                (self.send_packet)(Reliability::ReliableOrdered, ClientPacket::EncounterStart);

                // copy background
                let bg_props = self.area.map.background_properties();
                let background = bg_props.generate_background(game_io, &self.assets);

                // callback
                let event_sender = self.area.event_sender.clone();
                let statistics_callback = Box::new(move |statistics| {
                    let _ = event_sender.send(OverworldEvent::BattleStatistics(statistics));
                });

                // get package
                let encounter_package = package_path
                    .and_then(|path| self.encounter_packages.get(&path))
                    .map(|id| (PackageNamespace::Server, id.clone()));

                // create scene
                let mut battle_props = BattleProps::new_with_defaults(game_io, encounter_package);
                battle_props.meta.data = data;
                battle_props.meta.seed = seed;
                battle_props.meta.background = background;
                battle_props.statistics_callback = Some(statistics_callback);
                battle_props.comms.remote_id = battle_id;
                battle_props.comms.server =
                    Some((self.send_packet.clone(), self.battle_receiver.clone()));

                let player_data = &self.area.player_data;
                let player_setup = &mut battle_props.meta.player_setups[0];
                player_setup.emotion = player_data.emotion.clone();
                player_setup.health = player_data.health;
                player_setup.base_health = player_data.base_health;

                let props = NetplayProps {
                    battle_props,
                    remote_players,
                    fallback_address: self.server_address.clone(),
                };

                let scene = NetplayInitScene::new(game_io, props);

                let transition = crate::transitions::new_battle_init(game_io);
                let next_scene = NextScene::new_push(scene).with_transition(transition);
                self.next_scene_queue
                    .push_back((AutoEmote::Battle, next_scene));
            }

            ServerPacket::BattleMessage { battle_id, data } => {
                self.battle_sender.send((battle_id, data));
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
                map_color,
                animation,
                loop_animation,
            } => {
                let tile_position = Vec3::new(x, y, z);
                let position = self.area.map.tile_3d_to_world(tile_position);

                let entity = if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    if *entity != self.area.player_data.entity {
                        log::warn!("server used an actor_id that's already in use: {actor_id:?}");
                    }

                    // reuse entity
                    *entity
                } else {
                    // spawn actor
                    let texture = self.assets.texture(game_io, &texture_path);
                    let animator = Animator::load_new(&self.assets, &animation_path);

                    let entity = self
                        .area
                        .spawn_player_actor(game_io, texture, animator, position);

                    // mark as excluded as it was marked before spawn
                    if self.excluded_actors.contains_key(&actor_id) {
                        let entities = &mut self.area.entities;
                        Excluded::increment(entities, entity);
                    }

                    entity
                };

                if entity != self.area.player_data.entity {
                    let entities = &mut self.area.entities;

                    // tweak existing properties
                    let (sprite, direction, collider, map_marker) = entities
                        .query_one_mut::<(
                            &mut Sprite,
                            &mut Direction,
                            &mut ActorCollider,
                            &mut PlayerMapMarker,
                        )>(entity)
                        .unwrap();

                    sprite.set_scale(Vec2::new(scale_x, scale_y));
                    sprite.set_rotation(rotation);
                    *direction = initial_direction;
                    collider.solid = solid;
                    map_marker.color = map_color.into();

                    // setup remote player specific components
                    let _ = entities.insert(
                        entity,
                        (
                            InteractableActor(actor_id),
                            MovementInterpolator::new(game_io, position, initial_direction),
                            NameLabel(name),
                        ),
                    );

                    if let Some(state) = animation {
                        // setup initial animation
                        let (animator, movement_animator) = entities
                            .query_one_mut::<(&mut Animator, &mut MovementAnimator)>(entity)
                            .unwrap();

                        if animator.has_state(&state) {
                            animator.set_state(&state);
                            movement_animator.set_animation_enabled(false);

                            if loop_animation {
                                animator.set_loop_mode(AnimatorLoopMode::Loop);
                            }
                        }
                    }

                    if warp_in && !self.excluded_actors.contains_key(&actor_id) {
                        // create warp effect
                        WarpEffect::warp_in(
                            game_io,
                            &mut self.area,
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
                        let event_sender = self.area.event_sender.clone();

                        WarpEffect::warp_out(game_io, &mut self.area, entity, move |_, area| {
                            let _ = area.entities.despawn(entity);
                            area.despawn_sprite_attachments(entity);
                        });
                    } else {
                        let _ = self.area.entities.despawn(entity);
                        self.area.despawn_sprite_attachments(entity);
                    }
                }
            }
            ServerPacket::ActorSetName { actor_id, name } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let entities = &mut self.area.entities;

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
                    let entities = &mut self.area.entities;

                    let animating_properties = entities
                        .satisfies::<(&ActorPropertyAnimator)>(*entity)
                        .unwrap_or(false);

                    if let Ok(interpolator) =
                        entities.query_one_mut::<(&mut MovementInterpolator)>(*entity)
                    {
                        let tile_position = Vec3::new(x, y, z);
                        let position = self.area.map.tile_3d_to_world(tile_position);

                        if animating_properties {
                            interpolator.force_position(position)
                        } else if interpolator.is_movement_impossible(&self.area.map, position) {
                            if !self.excluded_actors.contains_key(&actor_id) {
                                interpolator.force_position(position);

                                WarpEffect::warp_full(
                                    game_io,
                                    &mut self.area,
                                    *entity,
                                    position,
                                    direction,
                                    |_, _| {},
                                );
                            }
                        } else {
                            interpolator.push(game_io, &self.area.map, position, direction);
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
                    type Query<'a> = (&'a mut Sprite, &'a mut Animator, &'a mut MovementAnimator);
                    let entities = &mut self.area.entities;
                    let (sprite, animator, movement_animator) =
                        entities.query_one_mut::<Query>(*entity).unwrap();

                    let texture = self.assets.texture(game_io, &texture_path);
                    sprite.set_texture(texture);
                    animator.load(&self.assets, &animation_path);

                    movement_animator.set_animation_enabled(true);
                }
            }
            ServerPacket::ActorEmote { actor_id, emote_id } => {
                if let Some(&entity) = self.actor_id_map.get_by_left(&actor_id) {
                    Emote::spawn_or_recycle(&mut self.area, entity, &emote_id);
                    Emote::animate_actor(
                        &mut self.area.entities,
                        entity,
                        &emote_id,
                        emote_id.ends_with("_LOOP") || emote_id.ends_with(" LOOP"),
                    );

                    if entity == self.area.player_data.entity {
                        let entities = &mut self.area.entities;
                        let (animator, position) = entities
                            .query_one_mut::<(&Animator, &Vec3)>(entity)
                            .unwrap();

                        if animator.has_state(&emote_id) {
                            let tile_position = self.area.map.world_3d_to_tile_space(*position);

                            (self.send_packet)(
                                Reliability::Reliable,
                                ClientPacket::AnimationUpdated {
                                    animation: emote_id,
                                    loop_animation: false,
                                    x: tile_position.x,
                                    y: tile_position.y,
                                    z: tile_position.z,
                                },
                            );
                        }
                    }
                }
            }
            ServerPacket::ActorAnimate {
                actor_id,
                state,
                loop_animation,
            } => {
                if let Some(&entity) = self.actor_id_map.get_by_left(&actor_id) {
                    Emote::animate_actor(&mut self.area.entities, entity, &state, loop_animation);
                }
            }
            ServerPacket::ActorPropertyKeyFrames {
                actor_id,
                keyframes,
            } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let mut property_animator = ActorPropertyAnimator::new();

                    if !self.area.visible || *entity != self.area.player_data.entity {
                        property_animator.set_audio_enabled(false);
                    }

                    let tile_size = self.area.map.tile_size().as_vec2();

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
                    let entities = &mut self.area.entities;
                    ActorPropertyAnimator::stop(entities, *entity);

                    // insert and start the new one
                    entities.insert_one(*entity, property_animator);

                    ActorPropertyAnimator::start(game_io, &self.assets, entities, *entity);
                }
            }
            ServerPacket::ActorMapColor { actor_id, color } => {
                if let Some(entity) = self.actor_id_map.get_by_left(&actor_id) {
                    let entities = &mut self.area.entities;

                    if let Ok(map_marker) = entities.query_one_mut::<&mut PlayerMapMarker>(*entity)
                    {
                        map_marker.color = Color::from(color);
                    }
                }
            }
            ServerPacket::SpriteCreated {
                sprite_id,
                attachment,
                definition,
            } => {
                self.sprite_id_map.entry(sprite_id).or_insert_with(|| {
                    let entities = &mut self.area.entities;
                    let assets = &self.assets;

                    let entity = match definition {
                        SpriteDefinition::Sprite {
                            texture_path,
                            animation_path,
                            animation_state,
                            animation_loops,
                        } => {
                            // set up sprite
                            let mut sprite = assets.new_sprite(game_io, &texture_path);

                            // set up animator
                            let mut animator = Animator::load_new(assets, &animation_path);
                            animator.set_state(&animation_state);

                            if animation_loops {
                                animator.set_loop_mode(AnimatorLoopMode::Loop);
                            } else if let Some(frame_list) = animator.frame_list(&animation_state) {
                                // end the state
                                animator.sync_time(frame_list.duration());
                            }

                            entities.spawn((animator, sprite))
                        }
                        SpriteDefinition::Text {
                            text: value,
                            text_style,
                            h_align,
                            v_align,
                        } => {
                            let text_style = TextStyle::from_blueprint(game_io, assets, text_style);
                            let mut text = Text::from(text_style);
                            text.text = value;
                            let alignment = AttachmentAlignment(h_align, v_align);

                            entities.spawn((text, alignment))
                        }
                    };

                    insert_attachment_bundle(entities, &self.actor_id_map, attachment, entity);

                    entity
                });
            }
            ServerPacket::SpriteAnimate {
                sprite_id,
                state,
                loop_animation,
            } => {
                if let Some(&entity) = self.sprite_id_map.get(&sprite_id) {
                    let entities = &mut self.area.entities;

                    if let Ok(animator) = entities.query_one_mut::<&mut Animator>(entity) {
                        animator.set_state(&state);

                        if loop_animation {
                            animator.set_loop_mode(AnimatorLoopMode::Loop);
                        }
                    }
                }
            }
            ServerPacket::SpriteDeleted { sprite_id } => {
                if let Some(entity) = self.sprite_id_map.remove(&sprite_id) {
                    let entities = &mut self.area.entities;
                    let _ = entities.despawn(entity);
                }
            }
            ServerPacket::SynchronizeUpdates => {
                self.synchronizing_packets = true;
            }
            ServerPacket::EndSynchronization => {
                self.synchronizing_packets = false;

                let packets = std::mem::take(&mut self.stored_packets);

                for packet in packets {
                    self.handle_packet(game_io, packet)
                }
            }
        }
    }

    fn push_textbox_interface_with_options(
        &mut self,
        game_io: &GameIO,
        interface: impl TextboxInterface + 'static,
        textbox_options: TextboxOptions,
    ) {
        // set avatar for the next interface
        self.menu_manager
            .set_next_avatar(game_io, &self.assets, textbox_options.mug.as_ref());

        // push interface
        self.menu_manager.push_textbox_interface(interface);

        // text style
        if let Some(blueprint) = textbox_options.text_style {
            let text_style = TextStyle::from_blueprint(game_io, &self.assets, blueprint);

            self.menu_manager.set_last_text_style(text_style);
        }
    }

    fn set_textbox_doorstop(&mut self) {
        self.doorstop_key.take();

        // keep the textbox open until the server receives our response or another textbox comes in
        let (doorstop_interface, doorstop_key) = TextboxDoorstop::new();
        self.doorstop_key = Some(doorstop_key);
        self.menu_manager.push_textbox_interface(doorstop_interface);
    }

    fn handle_events(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.area.event_receiver.try_recv() {
            match event {
                OverworldEvent::SystemMessage { message } => {
                    let interface = TextboxMessage::new(message);
                    self.menu_manager.use_player_avatar(game_io);
                    self.menu_manager.push_textbox_interface(interface);
                }
                OverworldEvent::Callback(callback) => {
                    callback(game_io, &mut self.area);
                }
                OverworldEvent::EmoteSelected(emote_id) => {
                    (self.send_packet)(
                        Reliability::ReliableOrdered,
                        ClientPacket::Emote { emote_id },
                    );
                }
                OverworldEvent::ItemUse(item_id) => {
                    (self.send_packet)(
                        Reliability::ReliableOrdered,
                        ClientPacket::ItemUse { item_id },
                    );
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
                    let player_data = &self.area.player_data;

                    let battle_stats = match statistics {
                        Some(statistics) => statistics,
                        None => BattleStatistics {
                            health: player_data.health,
                            emotion: player_data.emotion.clone(),
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
                        &mut self.area,
                        target_entity,
                        position,
                        direction,
                        |_, _| {},
                    );
                }
                OverworldEvent::PendingWarp { entity } => {
                    let map = &mut self.area.map;

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

                    self.next_scene_queue
                        .push_back((AutoEmote::None, next_scene));
                }
                OverworldEvent::Disconnected { message } => {
                    let event_sender = self.area.event_sender.clone();
                    let interface = TextboxMessage::new(message).with_callback(move || {
                        event_sender.send(OverworldEvent::Leave).unwrap();
                    });

                    self.menu_manager.use_player_avatar(game_io);
                    self.menu_manager.push_textbox_interface(interface);

                    self.doorstop_key.take();

                    self.connected = false;
                }
                OverworldEvent::PackageReferred(listing) => {
                    let scene = PackageScene::new(game_io, listing);
                    let transition = crate::transitions::new_sub_scene(game_io);
                    let next_scene = NextScene::new_push(scene).with_transition(transition);

                    self.next_scene_queue
                        .push_back((AutoEmote::Menu, next_scene));
                }
                OverworldEvent::NextScene(next) => {
                    self.next_scene_queue.push_back(next);
                }
                OverworldEvent::Leave => {
                    let transition = crate::transitions::new_connect(game_io);
                    self.next_scene_queue.push_back((
                        AutoEmote::None,
                        NextScene::new_pop().with_transition(transition),
                    ));
                }
            }
        }
    }

    fn handle_input(&mut self, game_io: &GameIO) {
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::ShoulderR) {
            self.menu_manager.use_player_avatar(game_io);
            let event_sender = self.area.event_sender.clone();

            let globals = Globals::from_resources(game_io);
            let message = globals.translate("server-leave-question");

            let interface = TextboxQuestion::new(game_io, message, move |response| {
                if response {
                    event_sender.send(OverworldEvent::Leave).unwrap();
                }
            });

            self.menu_manager.push_textbox_interface(interface);
        }

        if input_util.was_just_pressed(Input::Confirm) {
            self.handle_interaction(0);
        }

        if input_util.was_just_pressed(Input::ShoulderL) {
            self.handle_interaction(1);
        }
    }

    fn handle_interaction(&mut self, button: u8) {
        let player_data = &self.area.player_data;
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
        if let Some(actor_id) = player_data.actor_interaction {
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

        let entities = &mut self.area.entities;
        let player_entity = self.area.player_data.entity;
        let (position, direction) = entities
            .query_one_mut::<(&Vec3, &Direction)>(player_entity)
            .unwrap();

        let tile_position = self.area.map.world_3d_to_tile_space(*position);

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
        if game_io.is_in_transition() || !self.next_scene.is_none() {
            return;
        }

        let Some((auto_emote, next_scene)) = self.next_scene_queue.pop_front() else {
            return;
        };

        self.auto_emotes.set_auto_emote(auto_emote);

        if !matches!(&next_scene, NextScene::Push { .. }) && self.connected {
            // leaving as anything that isn't NextScene::Push will end up dropping this scene
            // notify the server
            (self.send_packet)(Reliability::ReliableOrdered, ClientPacket::Logout);
            self.connected = false;
        }

        self.next_scene = next_scene;
    }

    fn update_music(&mut self, game_io: &GameIO) {
        if game_io.is_in_transition() {
            return;
        }

        let globals = Globals::from_resources(game_io);

        if !globals.audio.is_music_playing() {
            globals.audio.restart_music();
        }

        let music_path = self.area.map.music_path();
        let current_music = globals.audio.current_music();

        if music_path.is_empty() {
            // try to play default music
            let playing_default_music =
                current_music.is_some_and(|b| globals.music.overworld.contains(&b));

            if !playing_default_music {
                globals.audio.pick_music(&globals.music.overworld, true);
            }
        } else {
            // try to play map music
            let sound_buffer = self.assets.audio(game_io, music_path);

            if current_music.as_ref() != Some(&sound_buffer) {
                globals.audio.play_music(&sound_buffer, true);
            }
        };
    }
}

impl Scene for OverworldOnlineScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.area.visible = true;

        self.auto_emotes.set_auto_emote(AutoEmote::None);

        // handle events triggered from other scenes
        // should be called before handling packets, but it's not necessary to do this every frame
        self.handle_events(game_io);

        let previous_player_id = self.area.player_data.package_id.clone();
        self.area.enter(game_io);

        // send preferences in case they've updated
        self.send_preferences(game_io);

        // send boosts before sending avatar data
        // allows the server to accurately calculate health
        self.send_boosts(game_io);

        if previous_player_id != self.area.player_data.package_id {
            // send avatar data
            self.send_avatar_data(game_io);
        }

        // enable audio on the player's ActorPropertyAnimator
        let entity = self.area.player_data.entity;
        let entities = &mut self.area.entities;

        if let Ok(property_animator) = entities.query_one_mut::<&mut ActorPropertyAnimator>(entity)
        {
            property_animator.set_audio_enabled(true);
        }
    }

    fn exit(&mut self, _game_io: &mut GameIO) {
        self.area.visible = false;

        // disable audio on the player's ActorPropertyAnimator
        let entity = self.area.player_data.entity;
        let entities = &mut self.area.entities;

        if let Ok(property_animator) = entities.query_one_mut::<&mut ActorPropertyAnimator>(entity)
        {
            property_animator.set_audio_enabled(false);
        }
    }

    fn continuous_update(&mut self, game_io: &mut GameIO) {
        self.handle_packets(game_io);
        self.auto_emotes
            .update(game_io, &mut self.area, &self.send_packet);

        if !self.area.visible {
            let area = &mut self.area;
            system_actor_property_animation(game_io, &self.assets, area);
            system_movement_interpolation(game_io, area);
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        let ui_is_locking_input = self.menu_manager.is_open();
        self.update_ui(game_io);

        if ui_is_locking_input {
            self.area.add_input_lock();
        }

        let area = &mut self.area;
        system_update_animation(&mut area.entities);

        let area = &mut self.area;
        system_player_movement(game_io, area, &self.assets);
        system_actor_property_animation(game_io, &self.assets, area);
        system_movement_interpolation(game_io, area);
        system_player_interaction(game_io, area);
        system_warp_effect(game_io, area);
        system_warp(game_io, area);
        system_movement_animation(&mut area.entities);
        system_movement(area);
        system_apply_animation(&mut area.entities);
        system_position(area);
        Emote::system(area);
        self.area.update(game_io);
        self.send_position(game_io);

        if !self.area.is_input_locked(game_io) {
            self.handle_input(game_io);
        }

        self.handle_events(game_io);
        self.handle_next_scene(game_io);

        if ui_is_locking_input {
            self.area.remove_input_lock();
        }

        self.update_music(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        if self.transferring {
            return;
        }

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.area.world_camera, SpriteColorMode::Multiply);

        if !self.menu_manager.is_blocking_view() {
            self.area.draw(game_io, render_pass, &mut sprite_queue);
        }

        // draw ui
        sprite_queue.update_camera(&self.area.ui_camera);

        if !self.menu_manager.is_blocking_hud() {
            // hide the map name while the textbox is visible
            let texbox_is_open = self.menu_manager.is_textbox_open();
            self.hud.set_map_name_visible(!texbox_is_open);

            // draw the hud
            self.hud.draw(game_io, &mut sprite_queue, &self.area.map);

            // draw hud attachments
            self.area
                .draw_screen_attachments::<HudAttachment>(game_io, &mut sprite_queue);
        }

        // draw menu, also draws widget attachments
        self.menu_manager
            .draw(game_io, render_pass, &mut sprite_queue, &self.area);

        render_pass.consume_queue(sprite_queue);
    }
}

fn ms_time(game_io: &GameIO) -> u64 {
    let duration = game_io.frame_start_instant() - game_io.game_start_instant();

    duration.as_millis() as u64
}
