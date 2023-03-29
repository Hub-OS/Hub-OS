use super::asset_manager::AssetManager;
use super::boot::Boot;
use super::client::{BattleTrackingInfo, Client};
use super::map::Map;
use super::*;
use crate::jobs::JobPromise;
use crate::threads::ThreadMessage;
use flume::Sender;
use generational_arena::Arena;
use packets::{Reliability, ServerPacket};
use std::borrow::Cow;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

pub struct Net {
    packet_orchestrator: Rc<RefCell<PacketOrchestrator>>,
    config: Rc<ServerConfig>,
    message_sender: Sender<ThreadMessage>,
    areas: HashMap<String, Area>,
    clients: HashMap<String, Client>,
    bots: HashMap<String, Actor>,
    sprites: HashMap<String, Sprite>,
    public_sprites: Arena<String>,
    asset_manager: AssetManager,
    active_plugin: usize,
    kick_list: Vec<Boot>,
    item_registry: HashMap<String, ItemDefinition>,
}

impl Net {
    pub fn new(
        packet_orchestrator: Rc<RefCell<PacketOrchestrator>>,
        config: Rc<ServerConfig>,
        message_sender: Sender<ThreadMessage>,
    ) -> Net {
        use super::asset::get_map_path;
        use std::fs::{read_dir, read_to_string};

        let mut asset_manager = AssetManager::new();
        asset_manager.load_assets_from_dir(std::path::Path::new("assets"));

        let mut areas = HashMap::new();
        let mut default_area_provided = false;

        for map_dir_entry in read_dir("./areas")
            .expect("Area folder missing! (./areas)")
            .flatten()
        {
            let map_path = map_dir_entry.path();
            let area_id = map_path
                .file_stem()
                .unwrap_or_default()
                .to_string_lossy()
                .into_owned();

            if let Ok(raw_map) = read_to_string(&map_path) {
                let mut map = Map::from(&raw_map);

                if area_id == "default" {
                    default_area_provided = true
                }

                let map_path = get_map_path(&area_id);
                let map_asset = map.generate_asset();

                asset_manager.set_asset(map_path, map_asset);
                areas.insert(area_id.clone(), Area::new(area_id, map));
            }
        }

        if !default_area_provided {
            panic!("No default (default.tmx) area data found");
        }

        Net {
            packet_orchestrator,
            config,
            message_sender,
            areas,
            clients: HashMap::new(),
            bots: HashMap::new(),
            sprites: HashMap::new(),
            public_sprites: Arena::new(),
            asset_manager,
            active_plugin: 0,
            kick_list: Vec::new(),
            item_registry: HashMap::new(),
        }
    }

    pub fn get_asset(&self, path: &str) -> Option<&Asset> {
        self.asset_manager.get_asset(path)
    }

    pub fn set_asset(&mut self, path: String, asset: Asset) {
        self.asset_manager.set_asset(path.clone(), asset);

        update_cached_clients(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &path,
        );
    }

    pub fn get_areas(&self) -> impl Iterator<Item = &Area> {
        self.areas.values()
    }

    pub fn get_area(&self, area_id: &str) -> Option<&Area> {
        self.areas.get(area_id)
    }

    pub fn get_area_mut(&mut self, area_id: &str) -> Option<&mut Area> {
        self.areas.get_mut(area_id)
    }

    pub fn add_area(&mut self, id: String, map: Map) {
        let mut map = map;

        if let Some(area) = self.areas.get_mut(&id) {
            area.set_map(map);
        } else {
            use super::asset::get_map_path;

            let map_path = get_map_path(&id);
            self.asset_manager.set_asset(map_path, map.generate_asset());
            self.areas.insert(id.clone(), Area::new(id, map));
        }
    }

    pub fn remove_area(&mut self, id: &str) {
        use super::asset::get_map_path;

        if id == "default" {
            log::error!("Can't delete default area!");
            return;
        }

        let map_path = get_map_path(id);
        self.asset_manager.remove_asset(&map_path);

        if let Some(area) = self.areas.remove(id) {
            let player_ids = area.connected_players();

            for player_id in player_ids {
                self.kick_player(player_id, "Area destroyed", true);
            }
        }
    }

    pub fn remove_asset(&mut self, path: &str) {
        self.asset_manager.remove_asset(path);
    }

    pub fn get_player(&self, id: &str) -> Option<&Actor> {
        self.clients.get(id).map(|client| &client.actor)
    }

    pub fn get_player_addr(&self, id: &str) -> Option<std::net::SocketAddr> {
        self.clients.get(id).map(|client| client.socket_address)
    }

    #[allow(dead_code)]
    pub(super) fn get_client(&self, id: &str) -> Option<&Client> {
        self.clients.get(id)
    }

    pub(super) fn get_client_mut(&mut self, id: &str) -> Option<&mut Client> {
        self.clients.get_mut(id)
    }

    pub fn require_asset(&mut self, area_id: &str, asset_path: &str) {
        if let Some(area) = self.areas.get_mut(area_id) {
            ensure_asset(
                &mut self.packet_orchestrator.borrow_mut(),
                self.config.max_payload_size,
                &self.asset_manager,
                &mut self.clients,
                area.connected_players(),
                asset_path,
            );

            area.require_asset(asset_path.to_string());
        }
    }

    pub fn play_sound(&mut self, area_id: &str, path: &str) {
        if let Some(area) = self.areas.get(area_id) {
            ensure_asset(
                &mut self.packet_orchestrator.borrow_mut(),
                self.config.max_payload_size,
                &self.asset_manager,
                &mut self.clients,
                area.connected_players(),
                path,
            );

            broadcast_to_area(
                &mut self.packet_orchestrator.borrow_mut(),
                area,
                Reliability::Reliable,
                ServerPacket::PlaySound {
                    path: path.to_string(),
                },
            )
        }
    }

    pub fn set_player_name(&mut self, id: &str, name: &str) {
        let client = match self.clients.get_mut(id) {
            Some(client) => client,
            None => return,
        };

        client.actor.name = name.to_string();

        if !client.ready {
            // skip if client has not even been sent to anyone yet
            return;
        }

        let area = match self.areas.get(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        let packet = ServerPacket::ActorSetName {
            actor_id: id.to_string(),
            name: name.to_string(),
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::Reliable,
            packet,
        );
    }

    pub fn set_player_avatar(&mut self, id: &str, texture_path: &str, animation_path: &str) {
        let client = match self.clients.get_mut(id) {
            Some(client) => client,
            None => return,
        };

        let area = match self.areas.get(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        client.actor.texture_path = texture_path.to_string();
        client.actor.animation_path = animation_path.to_string();

        // we'd normally skip if the player has not been sent to anyone yet
        // but for this we want to make sure the player sees this and updates their avatar
        // if the other players receive this, they'll just ignore it

        ensure_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            area.connected_players(),
            [texture_path, animation_path].iter(),
        );

        let packet = ServerPacket::ActorSetAvatar {
            actor_id: id.to_string(),
            texture_path: texture_path.to_string(),
            animation_path: animation_path.to_string(),
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::ReliableOrdered,
            packet,
        );
    }

    pub fn set_player_emote(&mut self, id: &str, emote_id: u8, use_custom_emotes: bool) {
        let client = match self.clients.get(id) {
            Some(client) => client,
            None => return,
        };

        let area = match self.areas.get(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        let packet = ServerPacket::ActorEmote {
            actor_id: id.to_string(),
            emote_id,
            use_custom_emotes,
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::Reliable,
            packet,
        );
    }

    pub fn exclusive_player_emote(
        &mut self,
        target_id: &str,
        emoter_id: &str,
        emote_id: u8,
        use_custom_emotes: bool,
    ) {
        let packet = ServerPacket::ActorEmote {
            actor_id: emoter_id.to_string(),
            emote_id,
            use_custom_emotes,
        };

        self.packet_orchestrator
            .borrow_mut()
            .send_by_id(target_id, Reliability::Reliable, packet);
    }

    pub fn set_player_map_color(&mut self, id: &str, color: (u8, u8, u8, u8)) {
        let client = match self.clients.get_mut(id) {
            Some(client) => client,
            None => return,
        };

        client.actor.map_color = color;

        let area = match self.areas.get(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::Reliable,
            ServerPacket::ActorMapColor {
                actor_id: id.to_string(),
                color,
            },
        );
    }

    pub fn animate_player(&mut self, id: &str, state: &str, loop_animation: bool) {
        let client = match self.clients.get(id) {
            Some(client) => client,
            None => return,
        };

        let area = match self.areas.get(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::Reliable,
            ServerPacket::ActorAnimate {
                actor_id: id.to_string(),
                state: state.to_string(),
                loop_animation,
            },
        );
    }

    pub fn animate_player_properties(&mut self, id: &str, animation: Vec<ActorKeyFrame>) {
        use std::collections::HashSet;

        let client = match self.clients.get_mut(id) {
            Some(client) => client,
            None => return,
        };

        let area = match self.areas.get(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        let mut asset_paths = HashSet::<&str>::new();

        // store final values for new players, also track assets
        for keyframe in &animation {
            for (property, _) in &keyframe.property_steps {
                match property {
                    ActorProperty::Animation(value) => {
                        client.actor.current_animation = Some(value.clone())
                    }
                    ActorProperty::ScaleX(value) => client.actor.scale_x = *value,
                    ActorProperty::ScaleY(value) => client.actor.scale_y = *value,
                    ActorProperty::Rotation(value) => client.actor.rotation = *value,
                    ActorProperty::Direction(value) => client.actor.direction = *value,
                    ActorProperty::SoundEffect(value) | ActorProperty::SoundEffectLoop(value) => {
                        if value.starts_with("/server/") {
                            asset_paths.insert(value);
                        }
                    }
                    _ => {}
                }
            }
        }

        ensure_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &[id.to_string()],
            asset_paths.iter(),
        );

        broadcast_actor_keyframes(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            id,
            animation,
        );
    }

    pub fn is_player_in_widget(&self, id: &str) -> bool {
        if let Some(client) = self.clients.get(id) {
            return client.is_in_widget();
        }

        false
    }

    pub fn is_player_shopping(&self, id: &str) -> bool {
        if let Some(client) = self.clients.get(id) {
            return client.is_shopping();
        }

        false
    }

    pub fn preload_asset_for_player(&mut self, id: &str, asset_path: &str) {
        let asset = match self.asset_manager.get_asset(asset_path) {
            Some(asset) => asset,
            None => return,
        };

        ensure_asset(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &[String::from(id)],
            asset_path,
        );

        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::Preload {
                asset_path: asset_path.to_string(),
                data_type: asset.data().data_type(),
            },
        );
    }

    pub fn play_sound_for_player(&mut self, id: &str, path: &str) {
        ensure_asset(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &[id.to_string()],
            path,
        );

        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::PlaySound {
                path: path.to_string(),
            },
        );
    }

    pub fn exclude_object_for_player(&mut self, id: &str, object_id: u32) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::ExcludeObject { id: object_id },
        );
    }

    pub fn include_object_for_player(&mut self, id: &str, object_id: u32) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::IncludeObject { id: object_id },
        );
    }

    pub fn exclude_actor_for_player(&mut self, id: &str, actor_id: &str) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::ExcludeActor {
                actor_id: actor_id.to_string(),
            },
        );
    }

    pub fn include_actor_for_player(&mut self, id: &str, actor_id: &str) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::IncludeActor {
                actor_id: actor_id.to_string(),
            },
        );
    }

    pub fn move_player_camera(&mut self, id: &str, x: f32, y: f32, z: f32, hold_time: f32) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::MoveCamera {
                x,
                y,
                z,
                hold_duration: hold_time,
            },
        );
    }

    pub fn slide_player_camera(&mut self, id: &str, x: f32, y: f32, z: f32, duration: f32) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::SlideCamera { x, y, z, duration },
        );
    }

    pub fn shake_player_camera(&mut self, id: &str, strength: f32, duration: f32) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::ShakeCamera { strength, duration },
        );
    }

    pub fn fade_player_camera(&mut self, id: &str, color: (u8, u8, u8, u8), duration: f32) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::FadeCamera { duration, color },
        );
    }

    pub fn track_with_player_camera(&mut self, id: &str, actor_id: &str) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::TrackWithCamera {
                actor_id: actor_id.to_string(),
            },
        );
    }

    pub fn enable_camera_controls(&mut self, id: &str, dist_x: f32, dist_y: f32) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::EnableCameraControls { dist_x, dist_y },
        )
    }

    pub fn unlock_player_camera(&mut self, id: &str) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::UnlockCamera,
        );
    }

    pub fn is_player_input_locked(&self, id: &str) -> bool {
        if let Some(client) = self.clients.get(id) {
            return client.input_locks > 0;
        }

        false
    }

    pub fn lock_player_input(&mut self, id: &str) {
        if let Some(client) = self.clients.get_mut(id) {
            client.input_locks += 1;

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::LockInput,
            );
        }
    }

    pub fn unlock_player_input(&mut self, id: &str) {
        if let Some(client) = self.clients.get_mut(id) {
            if client.input_locks == 0 {
                return;
            }

            client.input_locks -= 1;

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::UnlockInput,
            );
        }
    }

    pub fn teleport_player(
        &mut self,
        id: &str,
        warp: bool,
        x: f32,
        y: f32,
        z: f32,
        direction: Direction,
    ) {
        if let Some(client) = self.clients.get_mut(id) {
            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::Teleport {
                    warp,
                    x,
                    y,
                    z,
                    direction,
                },
            );

            // set their warp position
            // useful for moving players requesting/connecting
            client.warp_x = x;
            client.warp_y = y;
            client.warp_z = z;
            client.warp_direction = direction;

            // don't update internal position, allow the client to update this
        }
    }

    pub(crate) fn update_player_position(
        &mut self,
        id: &str,
        x: f32,
        y: f32,
        z: f32,
        direction: Direction,
    ) {
        let client = self.clients.get_mut(id).unwrap();

        client.actor.set_position(x, y, z);
        client.actor.set_direction(direction);

        // skip if client has not even been sent to anyone yet
        if !client.ready {
            return;
        }

        let idle_duration = client.actor.last_movement_time.elapsed().as_secs_f32();

        // skip if we've been sending idle packets for too long
        if idle_duration > self.config.max_idle_packet_duration {
            return;
        }

        let area = match self.areas.get(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        let packet = ServerPacket::ActorMove {
            actor_id: id.to_string(),
            x,
            y,
            z,
            direction,
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::UnreliableSequenced,
            packet,
        );
    }

    pub fn message_player(
        &mut self,
        id: &str,
        message: &str,
        mug_texture_path: &str,
        mug_animation_path: &str,
    ) {
        ensure_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &[id.to_string()],
            [mug_texture_path, mug_animation_path].iter(),
        );

        if let Some(client) = self.clients.get_mut(id) {
            client.widget_tracker.track_textbox(self.active_plugin);

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::Message {
                    message: message.to_string(),
                    mug_texture_path: mug_texture_path.to_string(),
                    mug_animation_path: mug_animation_path.to_string(),
                },
            );
        }
    }

    pub fn question_player(
        &mut self,
        id: &str,
        message: &str,
        mug_texture_path: &str,
        mug_animation_path: &str,
    ) {
        ensure_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &[id.to_string()],
            [mug_texture_path, mug_animation_path].iter(),
        );

        if let Some(client) = self.clients.get_mut(id) {
            client.widget_tracker.track_textbox(self.active_plugin);

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::Question {
                    message: message.to_string(),
                    mug_texture_path: mug_texture_path.to_string(),
                    mug_animation_path: mug_animation_path.to_string(),
                },
            );
        }
    }

    pub fn quiz_player(
        &mut self,
        id: &str,
        option_a: &str,
        option_b: &str,
        option_c: &str,
        mug_texture_path: &str,
        mug_animation_path: &str,
    ) {
        ensure_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &[id.to_string()],
            [mug_texture_path, mug_animation_path].iter(),
        );

        if let Some(client) = self.clients.get_mut(id) {
            client.widget_tracker.track_textbox(self.active_plugin);

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::Quiz {
                    option_a: option_a.to_string(),
                    option_b: option_b.to_string(),
                    option_c: option_c.to_string(),
                    mug_texture_path: mug_texture_path.to_string(),
                    mug_animation_path: mug_animation_path.to_string(),
                },
            );
        }
    }

    pub fn prompt_player(&mut self, id: &str, character_limit: u16, default_text: Option<&str>) {
        if let Some(client) = self.clients.get_mut(id) {
            client.widget_tracker.track_textbox(self.active_plugin);

            // reliability + id + type + u16 size
            let available_space = self.config.max_payload_size - 1 - 8 - 4 - 2 - 4;

            let character_limit = std::cmp::min(character_limit, available_space);

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::Prompt {
                    character_limit,
                    default_text: default_text.map(|s| s.to_string()),
                },
            );
        }
    }

    pub fn open_board(
        &mut self,
        player_id: &str,
        name: &str,
        color: (u8, u8, u8),
        posts: Vec<BbsPost>,
        open_instantly: bool,
    ) {
        let client = if let Some(client) = self.clients.get_mut(player_id) {
            client
        } else {
            return;
        };

        let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();

        client.widget_tracker.track_board(self.active_plugin);

        packet_orchestrator.send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::OpenBoard {
                topic: name.to_string(),
                color,
                posts,
                open_instantly,
            },
        );
    }

    pub fn prepend_posts(&mut self, player_id: &str, reference: Option<&str>, posts: Vec<BbsPost>) {
        let client = if let Some(client) = self.clients.get_mut(player_id) {
            client
        } else {
            return;
        };

        let reference = reference.map(|reference_str| reference_str.to_string());

        self.packet_orchestrator.borrow_mut().send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::PrependPosts { reference, posts },
        );
    }

    pub fn append_posts(&mut self, player_id: &str, reference: Option<&str>, posts: Vec<BbsPost>) {
        let client = if let Some(client) = self.clients.get_mut(player_id) {
            client
        } else {
            return;
        };

        let reference = reference.map(|reference_str| reference_str.to_string());

        self.packet_orchestrator.borrow_mut().send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::AppendPosts { reference, posts },
        );
    }

    pub fn remove_post(&mut self, player_id: &str, post_id: &str) {
        if let Some(client) = self.clients.get(player_id) {
            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::RemovePost {
                    id: post_id.to_string(),
                },
            );
        }
    }

    pub fn close_board(&mut self, player_id: &str) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            player_id,
            Reliability::ReliableOrdered,
            ServerPacket::CloseBoard,
        );
    }

    pub fn open_shop(
        &mut self,
        player_id: &str,
        items: Vec<ShopItem>,
        mug_texture_path: &str,
        mug_animation_path: &str,
    ) {
        ensure_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &[player_id.to_string()],
            [mug_texture_path, mug_animation_path].iter(),
        );

        let Some(client) = self.clients.get_mut(player_id) else {
            return;
        };

        client.widget_tracker.track_shop(self.active_plugin);

        let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();

        packet_orchestrator.send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::OpenShop {
                mug_texture_path: mug_texture_path.to_string(),
                mug_animation_path: mug_animation_path.to_string(),
            },
        );

        packet_orchestrator.send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::ShopInventory { items },
        );
    }

    pub fn set_shop_message(&mut self, player_id: &str, message: String) {
        let Some(client) = self.clients.get_mut(player_id) else {
            return;
        };

        let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
        packet_orchestrator.send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::ShopMessage { message },
        );
    }

    pub fn update_shop_item(&mut self, player_id: &str, item: ShopItem) {
        let Some(client) = self.clients.get_mut(player_id) else {
            return;
        };

        let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
        packet_orchestrator.send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::UpdateShopItem { item },
        );
    }

    pub fn remove_shop_item(&mut self, player_id: &str, id: String) {
        let Some(client) = self.clients.get_mut(player_id) else {
            return;
        };

        let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
        packet_orchestrator.send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::RemoveShopItem { id },
        );
    }

    pub fn is_player_battling(&self, id: &str) -> bool {
        if let Some(client) = self.clients.get(id) {
            return client.is_battling();
        }

        false
    }

    pub fn initiate_netplay(
        &mut self,
        ids: &[&str],
        package_path: Option<String>,
        data: Option<String>,
    ) {
        if let Some(package_path) = package_path.as_ref() {
            let player_ids: Vec<String> = ids.iter().map(|id| id.to_string()).collect();

            self.preload_package(&player_ids, package_path);
        }

        // todo: put these clients in slow mode

        let remote_players: Vec<_> = ids
            .iter()
            .enumerate()
            .flat_map(|(i, id)| {
                self.clients.get(*id).map(|client| RemotePlayerInfo {
                    index: i,
                    address: client.socket_address,
                    health: client.player_data.health,
                    base_health: client.player_data.base_health,
                })
            })
            .collect();

        let mut orchestrator = self.packet_orchestrator.borrow_mut();

        for (player_index, id) in ids.iter().enumerate() {
            if let Some(client) = self.clients.get_mut(*id) {
                let remote_addresses: Vec<_> = remote_players
                    .iter()
                    .filter(|info| info.address != client.socket_address)
                    .map(|info| info.address)
                    .collect();

                let tracking_info = BattleTrackingInfo {
                    plugin_index: self.active_plugin,
                    player_index,
                    remote_addresses,
                };

                client.battle_tracker.push_back(tracking_info);

                let info = remote_players
                    .iter()
                    .find(|info| info.index == player_index)
                    .unwrap();

                let remote_players: Vec<_> = remote_players
                    .iter()
                    .filter(|info| info.index != player_index)
                    .cloned()
                    .collect();

                let health = info.health;
                let base_health = info.base_health;

                orchestrator.send(
                    client.socket_address,
                    Reliability::ReliableOrdered,
                    ServerPacket::InitiateNetplay {
                        package_path: package_path.clone(),
                        data: data.clone(),
                        remote_players,
                        health,
                        base_health,
                    },
                );
            }
        }
    }

    pub fn set_mod_whitelist_for_player(&mut self, player_id: &str, whitelist_path: Option<&str>) {
        if let Some(whitelist_path) = whitelist_path {
            ensure_asset(
                &mut self.packet_orchestrator.borrow_mut(),
                self.config.max_payload_size,
                &self.asset_manager,
                &mut self.clients,
                &[String::from(player_id)],
                whitelist_path,
            );
        };

        self.packet_orchestrator.borrow_mut().send_by_id(
            player_id,
            Reliability::ReliableOrdered,
            ServerPacket::ModWhitelist {
                whitelist_path: whitelist_path.map(|s| s.to_string()),
            },
        );
    }

    pub fn set_mod_blacklist_for_player(&mut self, player_id: &str, blacklist_path: Option<&str>) {
        if let Some(blacklist_path) = blacklist_path {
            ensure_asset(
                &mut self.packet_orchestrator.borrow_mut(),
                self.config.max_payload_size,
                &self.asset_manager,
                &mut self.clients,
                &[String::from(player_id)],
                blacklist_path,
            );
        }

        self.packet_orchestrator.borrow_mut().send_by_id(
            player_id,
            Reliability::ReliableOrdered,
            ServerPacket::ModBlacklist {
                blacklist_path: blacklist_path.map(|s| s.to_string()),
            },
        );
    }

    pub fn offer_package(&mut self, player_id: &str, package_path: &str) {
        ensure_asset(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            &[String::from(player_id)],
            package_path,
        );

        let client = if let Some(client) = self.clients.get_mut(player_id) {
            client
        } else {
            return;
        };

        // send dependencies
        let dependency_chain = self
            .asset_manager
            .get_flattened_dependency_chain(package_path);

        let mut packets = Vec::new();
        packets.reserve_exact(dependency_chain.len());

        for asset_path in dependency_chain {
            let asset = if let Some(asset) = self.asset_manager.get_asset(asset_path) {
                asset
            } else {
                log::warn!("No asset found with path {:?}", asset_path);
                continue;
            };

            let package_info = if let Some(package_info) = asset.package_info() {
                package_info
            } else {
                log::warn!("{:?} is not a package", asset_path);
                continue;
            };

            packets.push(ServerPacket::OfferPackage {
                name: package_info.name.clone(),
                id: package_info.id.clone(),
                category: package_info.category,
                package_path: asset_path.to_string(),
            });
        }

        self.packet_orchestrator.borrow_mut().send_packets(
            client.socket_address,
            Reliability::ReliableOrdered,
            packets,
        );
    }

    pub fn preload_package(&mut self, player_ids: &[String], package_path: &str) {
        ensure_asset(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            player_ids,
            package_path,
        );

        // send dependencies
        let dependency_chain = self
            .asset_manager
            .get_flattened_dependency_chain(package_path);

        let mut packets = Vec::new();
        packets.reserve_exact(dependency_chain.len());

        for asset_path in dependency_chain {
            let asset = if let Some(asset) = self.asset_manager.get_asset(asset_path) {
                asset
            } else {
                log::warn!("No asset found with path {:?}", asset_path);
                continue;
            };

            let package_category = if let Some(package_info) = asset.package_info() {
                package_info.category
            } else {
                log::warn!("{:?} is not a package", asset_path);
                continue;
            };

            packets.push(ServerPacket::LoadPackage {
                package_path: asset_path.to_string(),
                category: package_category,
            });
        }

        let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
        use packets::serialize;
        let packets: Vec<_> = packets.into_iter().map(serialize).collect();

        for id in player_ids {
            packet_orchestrator.send_byte_packets_by_id(id, Reliability::ReliableOrdered, &packets);
        }
    }

    pub fn initiate_encounter(&mut self, player_id: &str, package_path: &str, data: Option<&str>) {
        self.preload_package(&[player_id.to_string()], package_path);

        let client = if let Some(client) = self.clients.get_mut(player_id) {
            client
        } else {
            return;
        };

        // update tracking
        let tracking_info = BattleTrackingInfo {
            plugin_index: self.active_plugin,
            ..Default::default()
        };

        client.battle_tracker.push_back(tracking_info);

        self.packet_orchestrator.borrow_mut().send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::InitiateEncounter {
                package_path: package_path.to_string(),
                data: data.map(|s| s.to_string()),
                health: client.player_data.health,
                base_health: client.player_data.base_health,
            },
        );
    }

    pub fn is_player_busy(&self, id: &str) -> bool {
        if let Some(client) = self.clients.get(id) {
            return client.is_busy();
        }

        true
    }

    pub fn get_player_data(&self, player_id: &str) -> Option<&PlayerData> {
        self.clients
            .get(player_id)
            .map(|client| &client.player_data)
    }

    pub(crate) fn update_player_data(
        &mut self,
        player_id: &str,
        avatar_name: String,
        element: String,
        base_health: i32,
    ) {
        let client = self.clients.get_mut(player_id).unwrap();
        let player_data = &mut client.player_data;

        player_data.avatar_name = avatar_name;
        player_data.element = element;
        player_data.base_health = base_health;
        player_data.health = base_health + player_data.health_boost;
    }

    pub fn set_player_health(&mut self, player_id: &str, health: i32) {
        if let Some(client) = self.clients.get_mut(player_id) {
            client.player_data.health = health;

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::Health { health },
            );
        }
    }

    pub fn set_player_base_health(&mut self, player_id: &str, base_health: i32) {
        if let Some(client) = self.clients.get_mut(player_id) {
            client.player_data.base_health = base_health;

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::BaseHealth { base_health },
            );
        }
    }

    pub fn set_player_emotion(&mut self, player_id: &str, emotion: u8) {
        if let Some(client) = self.clients.get_mut(player_id) {
            client.player_data.emotion = emotion;

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::Emotion { emotion },
            );
        }
    }

    pub fn set_player_money(&mut self, player_id: &str, money: u32) {
        if let Some(client) = self.clients.get_mut(player_id) {
            client.player_data.money = money;

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::Money { money },
            );
        }
    }

    pub fn get_item(&mut self, item_id: &str) -> Option<&ItemDefinition> {
        self.item_registry.get(item_id)
    }

    pub fn set_item(&mut self, item_id: String, item: ItemDefinition) {
        self.item_registry.insert(item_id, item);
    }

    pub fn give_player_item(&mut self, player_id: &str, item_id: String, count: usize) {
        let client = if let Some(client) = self.clients.get_mut(player_id) {
            client
        } else {
            return;
        };

        let item_definition =
            if let Some(item_definition) = self.item_registry.get(&item_id).cloned() {
                item_definition
            } else {
                log::warn!("No item found with id {:?}", item_id);
                return;
            };

        if !client.player_data.inventory.item_registered(&item_id) {
            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::RegisterItem {
                    id: item_id.clone(),
                    item_definition,
                },
            );
        }

        // must come after the item_registered check
        // will register the item with the inventory
        client.player_data.inventory.give_item(&item_id, count);

        self.packet_orchestrator.borrow_mut().send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::AddItem { id: item_id, count },
        );
    }

    pub fn remove_player_item(&mut self, player_id: &str, item_id: String, count: usize) {
        if let Some(client) = self.clients.get_mut(player_id) {
            client.player_data.inventory.remove_item(&item_id, count);

            self.packet_orchestrator.borrow_mut().send(
                client.socket_address,
                Reliability::ReliableOrdered,
                ServerPacket::RemoveItem { id: item_id, count },
            );
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub fn transfer_player(
        &mut self,
        id: &str,
        area_id: &str,
        warp_in: bool,
        x: f32,
        y: f32,
        z: f32,
        direction: Direction,
    ) {
        if self.areas.get(area_id).is_none() {
            // non existent area
            return;
        }

        let client = match self.clients.get_mut(id) {
            Some(client) => client,
            None => return,
        };

        let previous_area = match self.areas.get_mut(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        client.warp_in = warp_in;
        client.warp_x = x;
        client.warp_y = y;
        client.warp_z = z;
        client.warp_direction = direction;

        if !previous_area.connected_players().contains(&id.to_string()) {
            // client has not been added to any area yet
            // assume client was transferred on initial connection by a plugin
            client.actor.area_id = area_id.to_string();
            return;
        }

        previous_area.remove_player(id);

        self.packet_orchestrator
            .borrow_mut()
            .leave_room(client.socket_address, previous_area.id());

        client.warp_area = area_id.to_string();

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            previous_area,
            Reliability::ReliableOrdered,
            ServerPacket::ActorDisconnected {
                actor_id: id.to_string(),
                warp_out: warp_in,
            },
        );

        if warp_in {
            self.packet_orchestrator.borrow_mut().send_by_id(
                id,
                Reliability::ReliableOrdered,
                ServerPacket::TransferWarp,
            );
        } else {
            self.complete_transfer(id)
        }
    }

    pub(super) fn complete_transfer(&mut self, player_id: &str) {
        let client = if let Some(client) = self.clients.get_mut(player_id) {
            client
        } else {
            return;
        };

        self.packet_orchestrator.borrow_mut().send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::TransferStart,
        );

        let area_id = client.warp_area.clone();
        let area = if let Some(area) = self.areas.get_mut(&area_id) {
            area
        } else {
            self.kick_player(player_id, "Area destroyed", true);
            return;
        };

        let texture_path = client.actor.texture_path.clone();
        let animation_path = client.actor.animation_path.clone();

        self.packet_orchestrator
            .borrow_mut()
            .join_room(client.socket_address, area_id.clone());

        ensure_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            area.connected_players(),
            [texture_path.as_str(), animation_path.as_str()].iter(),
        );

        area.add_player(player_id.to_string());
        self.send_area(player_id, &area_id);

        let mut client = self.clients.get_mut(player_id).unwrap();

        client.actor.area_id = area_id.to_string();
        client.transferring = true;
        client.ready = false;

        self.packet_orchestrator.borrow_mut().send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::Teleport {
                warp: false,
                x: client.warp_x,
                y: client.warp_y,
                z: client.warp_z,
                direction: client.warp_direction,
            },
        );

        self.packet_orchestrator.borrow_mut().send(
            client.socket_address,
            Reliability::ReliableOrdered,
            ServerPacket::TransferComplete {
                warp_in: client.warp_in,
                direction: client.warp_direction,
            },
        );
    }

    pub fn transfer_server(&mut self, id: &str, address: &str, data: &str, warp_out: bool) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::TransferServer {
                address: address.to_string(),
                data: data.to_string(),
                warp_out,
            },
        );

        self.kick_player(id, "Transferred", warp_out);
    }

    pub fn request_authorization(&mut self, id: &str, address: &str, data: &[u8]) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            id,
            Reliability::ReliableOrdered,
            ServerPacket::Authorize {
                address: address.to_string(),
                data: data.to_vec(),
            },
        );
    }

    pub fn kick_player(&mut self, id: &str, reason: &str, warp_out: bool) {
        if let Some(client) = self.clients.get(id) {
            self.kick_list.push(Boot {
                socket_address: client.socket_address,
                reason: reason.to_string(),
                warp_out,
            });
        }
    }

    pub(super) fn take_kick_list(&mut self) -> Vec<Boot> {
        let mut out = Vec::new();

        std::mem::swap(&mut self.kick_list, &mut out);

        out
    }

    pub(super) fn add_client(
        &mut self,
        socket_address: std::net::SocketAddr,
        name: String,
        identity: Vec<u8>,
    ) -> String {
        let area_id = String::from("default");
        let area = self.get_area_mut(&area_id).unwrap();
        let map = area.map();
        let (spawn_x, spawn_y, spawn_z) = map.spawn_position();
        let spawn_direction = map.spawn_direction();

        let client = Client::new(
            socket_address,
            name,
            identity,
            area_id.clone(),
            spawn_x,
            spawn_y,
            spawn_z,
            spawn_direction,
        );

        let id = client.actor.id.clone();

        let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();
        packet_orchestrator.register_client(client.socket_address, id.clone());

        self.clients.insert(id.clone(), client);

        id
    }

    pub(super) fn store_player_assets(&mut self, player_id: &str) -> Option<(String, String)> {
        use super::client::find_longest_frame_length;

        let client = self.clients.get_mut(player_id).unwrap();

        let texture_data = client.texture_buffer.clone();
        let animation_data = String::from_utf8_lossy(&client.animation_buffer).into_owned();
        let mugshot_texture_data = client.mugshot_texture_buffer.clone();
        let mugshot_animation_data =
            String::from_utf8_lossy(&client.mugshot_animation_buffer).into_owned();

        // reset buffers to store new data later
        client.texture_buffer.clear();
        client.animation_buffer.clear();
        client.mugshot_texture_buffer.clear();
        client.mugshot_animation_buffer.clear();

        let avatar_dimensions_limit = self.config.avatar_dimensions_limit;

        if find_longest_frame_length(&animation_data) > avatar_dimensions_limit {
            let reason = format!(
                "Avatar has frames larger than limit {}x{}",
                avatar_dimensions_limit, avatar_dimensions_limit
            );

            self.kick_player(player_id, &reason, true);

            return None;
        }

        let texture_path = asset::get_player_texture_path(player_id);
        let animation_path = asset::get_player_animation_path(player_id);
        let mugshot_texture_path = asset::get_player_mugshot_texture_path(player_id);
        let mugshot_animation_path = asset::get_player_mugshot_animation_path(player_id);

        let player_assets = [
            (texture_path.clone(), AssetData::Texture(texture_data)),
            (
                animation_path.clone(),
                AssetData::compress_text(animation_data),
            ),
            (
                mugshot_texture_path,
                AssetData::Texture(mugshot_texture_data),
            ),
            (
                mugshot_animation_path,
                AssetData::compress_text(mugshot_animation_data),
            ),
        ];

        for (path, data) in player_assets.into_iter() {
            self.set_asset(
                path,
                Asset {
                    data,
                    alternate_names: Vec::new(),
                    dependencies: Vec::new(),
                    last_modified: 0,
                    cachable: true,
                    cache_to_disk: false,
                },
            );
        }

        Some((texture_path, animation_path))
    }

    pub(super) fn spawn_client(&mut self, player_id: &str) {
        let client = self.clients.get(player_id).unwrap();
        let area_id = client.actor.area_id.clone();
        let texture_path = client.actor.texture_path.clone();
        let animation_path = client.actor.animation_path.clone();

        let area = match self.areas.get_mut(&area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        area.add_player(client.actor.id.clone());

        {
            let packet_orchestrator = &mut self.packet_orchestrator.borrow_mut();
            packet_orchestrator.join_room(client.socket_address, area_id.clone());

            ensure_assets(
                packet_orchestrator,
                self.config.max_payload_size,
                &self.asset_manager,
                &mut self.clients,
                area.connected_players(),
                [texture_path.as_str(), animation_path.as_str()].iter(),
            );
        }

        self.send_area(player_id, &area_id);

        let client = self.clients.get_mut(player_id).unwrap();

        let packet = ServerPacket::Login {
            actor_id: player_id.to_string(),
            warp_in: client.warp_in,
            spawn_x: client.warp_x,
            spawn_y: client.warp_y,
            spawn_z: client.warp_z,
            spawn_direction: client.warp_direction,
        };

        self.packet_orchestrator.borrow_mut().send(
            client.socket_address,
            Reliability::ReliableOrdered,
            packet,
        );
    }

    pub(super) fn connect_client(&mut self, player_id: &str) {
        self.packet_orchestrator.borrow_mut().send_by_id(
            player_id,
            Reliability::ReliableOrdered,
            ServerPacket::CompleteConnection,
        );
    }

    fn send_area(&mut self, player_id: &str, area_id: &str) {
        use super::asset::get_map_path;

        let area = self.areas.get(area_id).unwrap();

        let mut packets: Vec<ServerPacket> = Vec::new();
        let mut asset_paths: Vec<String> = area.required_assets().clone();

        // send map
        let map_path = get_map_path(area_id);
        asset_paths.push(map_path.clone());
        packets.push(ServerPacket::MapUpdate { map_path });

        // send clients
        for other_player_id in area.connected_players() {
            if other_player_id == player_id {
                continue;
            }

            let other_client = self.clients.get(other_player_id).unwrap();
            let actor = &other_client.actor;

            asset_paths.push(actor.texture_path.clone());
            asset_paths.push(actor.animation_path.clone());

            packets.push(actor.create_spawn_packet(actor.x, actor.y, actor.z, false));
        }

        // send bots
        for bot_id in area.connected_bots() {
            let bot = self.bots.get(bot_id).unwrap();

            asset_paths.push(bot.texture_path.clone());
            asset_paths.push(bot.animation_path.clone());

            packets.push(bot.create_spawn_packet(bot.x, bot.y, bot.z, false));
        }

        // send public sprites
        for (_, sprite_id) in &self.public_sprites {
            let sprite = &self.sprites[sprite_id];

            let Some(scope) = Self::resolve_sprite_packet_scope(&self.clients, &self.bots, sprite) else {
                continue;
            };

            if !scope.is_visible_to(area_id, player_id) {
                continue;
            }

            asset_paths.push(sprite.definition.animation_path.clone());
            asset_paths.push(sprite.definition.texture_path.clone());

            packets.push(ServerPacket::SpriteCreated {
                sprite_id: sprite_id.clone(),
                sprite_definition: sprite.definition.clone(),
            });
        }

        if let Some(custom_emotes_path) = &self.config.custom_emotes_path {
            asset_paths.push(custom_emotes_path.clone());
            packets.push(ServerPacket::CustomEmotesPath {
                asset_path: custom_emotes_path.clone(),
            });
        }

        // build and collect packets to avoid lifetime overlap
        use packets::serialize;
        let packets: Vec<Vec<u8>> = packets.iter().map(serialize).collect();

        // send asset_packets before anything else
        let asset_recievers = vec![player_id.to_string()];

        for asset_path in asset_paths {
            ensure_asset(
                &mut self.packet_orchestrator.borrow_mut(),
                self.config.max_payload_size,
                &self.asset_manager,
                &mut self.clients,
                &asset_recievers[..],
                &asset_path,
            );
        }

        self.packet_orchestrator
            .borrow_mut()
            .send_byte_packets_by_id(player_id, Reliability::ReliableOrdered, &packets);
    }

    // handles first join and completed transfer
    pub(super) fn mark_client_ready(&mut self, id: &str) {
        use packets::serialize;

        let client = match self.clients.get_mut(id) {
            Some(client) => client,
            None => return,
        };

        let area = match self.areas.get(&client.actor.area_id) {
            Some(area) => area,
            None => return, // area deleted, should be getting kicked
        };

        client.ready = true;
        client.transferring = false;

        let packet = client.actor.create_spawn_packet(
            client.warp_x,
            client.warp_y,
            client.warp_z,
            client.warp_in,
        );

        let packet_bytes = serialize(packet);

        self.packet_orchestrator
            .borrow_mut()
            .broadcast_bytes_to_room(area.id(), Reliability::ReliableOrdered, packet_bytes);
    }

    pub(super) fn remove_player(&mut self, id: &str, warp_out: bool) {
        let client = match self.clients.remove(id) {
            Some(client) => client,
            None => return,
        };

        // remove assets
        let remove_list = [
            asset::get_player_texture_path(id),
            asset::get_player_animation_path(id),
            asset::get_player_mugshot_animation_path(id),
            asset::get_player_mugshot_texture_path(id),
        ];

        for asset_path in remove_list.iter() {
            self.asset_manager.remove_asset(asset_path);
        }

        // delete sprites
        for id in client.actor.child_sprites {
            let Some(sprite) = self.sprites.remove(&id) else {
                continue;
            };

            if let Some(index) = sprite.public_sprites_index {
                self.public_sprites.remove(index);
            }
        }

        // remove from packet_orchestrator
        let mut packet_orchestrator = self.packet_orchestrator.borrow_mut();

        packet_orchestrator.unregister_client(client.socket_address);

        // remove player from the area
        let area = match self.areas.get_mut(&client.actor.area_id) {
            Some(area) => area,
            None => return,
        };

        area.remove_player(&client.actor.id);

        let packet = ServerPacket::ActorDisconnected {
            actor_id: id.to_string(),
            warp_out,
        };

        broadcast_to_area(
            &mut packet_orchestrator,
            area,
            Reliability::ReliableOrdered,
            packet,
        );
    }

    pub fn get_bot(&self, id: &str) -> Option<&Actor> {
        self.bots.get(id)
    }

    pub fn add_bot(&mut self, bot: Actor, warp_in: bool) {
        if self.bots.contains_key(&bot.id) {
            log::warn!("A bot with id {:?} already exists!", bot.id);
            return;
        }

        if self.clients.contains_key(&bot.id) {
            log::warn!("A player with id {:?} exists, can't create bot!", bot.id);
            return;
        }

        if let Some(area) = self.areas.get_mut(&bot.area_id) {
            area.add_bot(bot.id.clone());

            let packet = bot.create_spawn_packet(bot.x, bot.y, bot.z, warp_in);

            ensure_assets(
                &mut self.packet_orchestrator.borrow_mut(),
                self.config.max_payload_size,
                &self.asset_manager,
                &mut self.clients,
                area.connected_players(),
                [bot.texture_path.as_str(), bot.animation_path.as_str()].iter(),
            );

            broadcast_to_area(
                &mut self.packet_orchestrator.borrow_mut(),
                area,
                Reliability::ReliableOrdered,
                packet,
            );

            self.bots.insert(bot.id.clone(), bot);
        }
    }

    pub fn remove_bot(&mut self, id: &str, warp_out: bool) {
        let bot = match self.bots.remove(id) {
            Some(bot) => bot,
            None => return,
        };

        // delete sprites
        for id in bot.child_sprites {
            let Some(sprite) = self.sprites.remove(&id) else {
                continue;
            };

            if let Some(index) = sprite.public_sprites_index {
                self.public_sprites.remove(index);
            }
        }

        // remove bot from the area
        let area = match self.areas.get_mut(&bot.area_id) {
            Some(area) => area,
            None => return,
        };

        area.remove_bot(&bot.id);

        let packet = ServerPacket::ActorDisconnected {
            actor_id: id.to_string(),
            warp_out,
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::ReliableOrdered,
            packet,
        );
    }

    pub fn set_bot_name(&mut self, id: &str, name: &str) {
        let bot = match self.bots.get_mut(id) {
            Some(bot) => bot,
            None => return,
        };

        bot.name = name.to_string();

        let area = match self.areas.get(&bot.area_id) {
            Some(area) => area,
            None => return,
        };

        let packet = ServerPacket::ActorSetName {
            actor_id: id.to_string(),
            name: name.to_string(),
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::Reliable,
            packet,
        );
    }

    pub fn move_bot(&mut self, id: &str, x: f32, y: f32, z: f32) {
        if let Some(bot) = self.bots.get_mut(id) {
            let updated_direction = Direction::from_offset((x - bot.x, y - bot.y));

            if !matches!(updated_direction, Direction::None) {
                bot.set_direction(updated_direction);
            }

            bot.set_position(x, y, z);
        }
    }

    pub fn set_bot_direction(&mut self, id: &str, direction: Direction) {
        if let Some(bot) = self.bots.get_mut(id) {
            bot.set_direction(direction);
        }
    }

    pub fn set_bot_avatar(&mut self, id: &str, texture_path: &str, animation_path: &str) {
        let bot = match self.bots.get_mut(id) {
            Some(bot) => bot,
            None => return,
        };

        bot.texture_path = texture_path.to_string();
        bot.animation_path = animation_path.to_string();

        update_cached_clients(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            texture_path,
        );

        update_cached_clients(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            animation_path,
        );

        let area = match self.areas.get(&bot.area_id) {
            Some(area) => area,
            None => return,
        };

        let packet = ServerPacket::ActorSetAvatar {
            actor_id: id.to_string(),
            texture_path: texture_path.to_string(),
            animation_path: animation_path.to_string(),
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::ReliableOrdered,
            packet,
        );
    }

    pub fn set_bot_emote(&mut self, id: &str, emote_id: u8, use_custom_emotes: bool) {
        let bot = match self.bots.get(id) {
            Some(bot) => bot,
            None => return,
        };

        let area = match self.areas.get(&bot.area_id) {
            Some(area) => area,
            None => return,
        };

        let packet = ServerPacket::ActorEmote {
            actor_id: id.to_string(),
            emote_id,
            use_custom_emotes,
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::Reliable,
            packet,
        );
    }

    pub fn set_bot_map_color(&mut self, id: &str, color: (u8, u8, u8, u8)) {
        let bot = match self.bots.get_mut(id) {
            Some(bot) => bot,
            None => return,
        };

        bot.map_color = color;

        let area = match self.areas.get(&bot.area_id) {
            Some(area) => area,
            None => return,
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::Reliable,
            ServerPacket::ActorMapColor {
                actor_id: id.to_string(),
                color,
            },
        );
    }

    pub fn animate_bot(&mut self, id: &str, name: &str, loop_animation: bool) {
        let bot = match self.bots.get(id) {
            Some(bot) => bot,
            None => return,
        };

        let area = match self.areas.get(&bot.area_id) {
            Some(area) => area,
            None => return,
        };

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::Reliable,
            ServerPacket::ActorAnimate {
                actor_id: id.to_string(),
                state: name.to_string(),
                loop_animation,
            },
        );
    }

    pub fn animate_bot_properties(&mut self, id: &str, animation: Vec<ActorKeyFrame>) {
        if let Some(bot) = self.bots.get_mut(id) {
            // store final values for new players
            let area = match self.areas.get(&bot.area_id) {
                Some(area) => area,
                None => return,
            };

            let mut final_x = bot.x;
            let mut final_y = bot.y;
            let mut final_z = bot.z;

            for keyframe in &animation {
                for (property, _) in &keyframe.property_steps {
                    match property {
                        ActorProperty::Animation(value) => {
                            bot.current_animation = Some(value.clone())
                        }
                        ActorProperty::X(value) => final_x = *value,
                        ActorProperty::Y(value) => final_y = *value,
                        ActorProperty::Z(value) => final_z = *value,
                        ActorProperty::ScaleX(value) => bot.scale_x = *value,
                        ActorProperty::ScaleY(value) => bot.scale_y = *value,
                        ActorProperty::Rotation(value) => bot.rotation = *value,
                        ActorProperty::Direction(value) => bot.direction = *value,
                        _ => {}
                    }
                }
            }

            // set position directly, to avoid reseting the animation
            bot.x = final_x;
            bot.y = final_y;
            bot.z = final_z;

            broadcast_actor_keyframes(
                &mut self.packet_orchestrator.borrow_mut(),
                area,
                id,
                animation,
            );
        }
    }

    pub fn transfer_bot(&mut self, id: &str, area_id: &str, warp_in: bool, x: f32, y: f32, z: f32) {
        if self.areas.get(area_id).is_none() {
            // non existent area
            return;
        }

        let Some(bot) = self.bots.get_mut(id) else {
            return;
        };

        if let Some(previous_area) = self.areas.get_mut(&bot.area_id) {
            previous_area.remove_bot(id);

            broadcast_to_area(
                &mut self.packet_orchestrator.borrow_mut(),
                previous_area,
                Reliability::ReliableOrdered,
                ServerPacket::ActorDisconnected {
                    actor_id: id.to_string(),
                    warp_out: warp_in,
                },
            );
        }

        bot.area_id = area_id.to_string();
        bot.x = x;
        bot.y = y;
        bot.z = z;

        let area = self.areas.get_mut(area_id).unwrap();
        area.add_bot(id.to_string());

        ensure_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &mut self.clients,
            area.connected_players(),
            [bot.texture_path.as_str(), bot.animation_path.as_str()].iter(),
        );

        broadcast_to_area(
            &mut self.packet_orchestrator.borrow_mut(),
            area,
            Reliability::ReliableOrdered,
            bot.create_spawn_packet(bot.x, bot.y, bot.z, warp_in),
        );

        let bot = self.bots.get(id).unwrap();
        let area = self.areas.get(area_id).unwrap();

        for sprite_id in &bot.child_sprites {
            let sprite = self.sprites.get(id).unwrap();

            ensure_assets(
                &mut self.packet_orchestrator.borrow_mut(),
                self.config.max_payload_size,
                &self.asset_manager,
                &mut self.clients,
                area.connected_players(),
                [
                    &sprite.definition.texture_path,
                    &sprite.definition.animation_path,
                ],
            );

            let Some(scope) = Self::resolve_sprite_packet_scope(&self.clients, &self.bots, sprite) else {
                continue;
            };

            broadcast_to_scope(
                &mut self.packet_orchestrator.borrow_mut(),
                scope,
                &self.areas,
                Reliability::ReliableOrdered,
                ServerPacket::SpriteCreated {
                    sprite_id: sprite_id.clone(),
                    sprite_definition: sprite.definition.clone(),
                },
            );
        }
    }

    pub fn create_sprite(
        &mut self,
        client_id_restriction: Option<String>,
        sprite_definition: SpriteDefinition,
    ) -> String {
        use uuid::Uuid;

        let id = Uuid::new_v4().to_string();

        let public_sprites_index = if client_id_restriction.is_none() {
            Some(self.public_sprites.insert(id.clone()))
        } else {
            None
        };

        if let SpriteParent::Actor(actor_id) = &sprite_definition.parent {
            if let Some(client) = self.clients.get_mut(actor_id) {
                client.actor.child_sprites.push(id.clone());
            } else if let Some(bot) = self.bots.get_mut(actor_id) {
                bot.child_sprites.push(id.clone());
            }
        }

        // create sprite
        let sprite = Sprite {
            definition: sprite_definition.clone(),
            public_sprites_index,
            client_id_restriction,
        };

        // resolve relevant scope for notifying creation
        let scope = Self::resolve_sprite_packet_scope(&self.clients, &self.bots, &sprite)
            .map(|scope| scope.into_owned());

        // store sprite
        self.sprites.insert(id.clone(), sprite);

        let Some(scope) = scope else {
            // no clients to send packets to
            return id;
        };

        // send assets
        ensure_scope_has_assets(
            &mut self.packet_orchestrator.borrow_mut(),
            self.config.max_payload_size,
            &self.asset_manager,
            &self.areas,
            &mut self.clients,
            &scope,
            [
                &sprite_definition.animation_path,
                &sprite_definition.texture_path,
            ],
        );

        // send creation packet
        let packet = ServerPacket::SpriteCreated {
            sprite_id: id.clone(),
            sprite_definition,
        };

        broadcast_to_scope(
            &mut self.packet_orchestrator.borrow_mut(),
            scope,
            &self.areas,
            Reliability::ReliableOrdered,
            packet,
        );

        id
    }

    pub fn animate_sprite(&mut self, sprite_id: String, state: String, loop_animation: bool) {
        self.message_sprite_aware(sprite_id, |sprite_id| ServerPacket::SpriteAnimate {
            sprite_id,
            state,
            loop_animation,
        });
    }

    pub fn delete_sprite(&mut self, sprite_id: String) {
        self.message_sprite_aware(sprite_id.clone(), |sprite_id| ServerPacket::SpriteDeleted {
            sprite_id,
        });

        let Some(sprite) = self.sprites.remove(&sprite_id) else {
            return;
        };

        if let Some(index) = sprite.public_sprites_index {
            self.public_sprites.remove(index);
        }

        if let SpriteParent::Actor(actor_id) = &sprite.definition.parent {
            let mut empty = Vec::new();

            let id_list = if let Some(client) = self.clients.get_mut(actor_id) {
                &mut client.actor.child_sprites
            } else if let Some(bot) = self.bots.get_mut(actor_id) {
                &mut bot.child_sprites
            } else {
                &mut empty
            };

            if let Some(index) = id_list.iter().position(|id| id == &sprite_id) {
                id_list.remove(index);
            }
        }
    }

    fn message_sprite_aware(
        &mut self,
        sprite_id: String,
        callback: impl FnOnce(String) -> ServerPacket,
    ) {
        let Some(sprite) = self.sprites.get(&sprite_id) else {
            return;
        };

        let Some(scope) = Self::resolve_sprite_packet_scope(&self.clients, &self.bots, sprite) else {
            return;
        };

        let packet = callback(sprite_id);

        broadcast_to_scope(
            &mut self.packet_orchestrator.borrow_mut(),
            scope,
            &self.areas,
            Reliability::ReliableOrdered,
            packet,
        );
    }

    fn resolve_sprite_packet_scope<'a>(
        clients: &'a HashMap<String, Client>,
        bots: &'a HashMap<String, Actor>,
        sprite: &'a Sprite,
    ) -> Option<PacketScope<'a>> {
        if let Some(client_id) = &sprite.client_id_restriction {
            // send the packet just to this client
            return Some(PacketScope::Client(Cow::Borrowed(client_id)));
        }

        // assume public as there's no client_id_restriction

        if let SpriteParent::Actor(actor_id) = &sprite.definition.parent {
            // send just to clients in the same area as the actor
            let client_area_id = clients.get(actor_id).map(|client| &client.actor.area_id);
            let area_id = client_area_id.or_else(|| bots.get(actor_id).map(|actor| &actor.area_id));

            return Some(PacketScope::Area(Cow::Borrowed(area_id?)));
        }

        // broadcast to everyone
        Some(PacketScope::All)
    }

    pub fn request_update_synchronization(&mut self) {
        self.packet_orchestrator
            .borrow_mut()
            .request_update_synchronization();
    }

    pub fn request_disable_update_synchronization(&mut self) {
        self.packet_orchestrator
            .borrow_mut()
            .request_disable_update_synchronization();
    }

    pub fn poll_server(&mut self, address: String) -> JobPromise {
        let message_sender = self.message_sender.clone();
        let promise = JobPromise::new();
        let out_promise = promise.clone();

        async_std::task::spawn(async move {
            if let Some(socket_address) =
                packets::address_parsing::resolve_socket_addr(&address).await
            {
                message_sender
                    .send(ThreadMessage::PollServer {
                        socket_address,
                        promise,
                    })
                    .unwrap();
            }
        });

        out_promise
    }

    pub fn message_server(&mut self, address: String, data: Vec<u8>) {
        let message_sender = self.message_sender.clone();

        async_std::task::spawn(async move {
            if let Some(socket_address) =
                packets::address_parsing::resolve_socket_addr(&address).await
            {
                message_sender
                    .send(ThreadMessage::MessageServer {
                        socket_address,
                        data,
                    })
                    .unwrap();
            }
        });
    }

    // ugly opengl like context storing
    // needed to correctly track message owners send without adding extra parameters
    // luckily not visible to plugin authors
    pub(super) fn set_active_plugin(&mut self, active_plugin: usize) {
        self.active_plugin = active_plugin;
    }

    pub(super) fn tick(&mut self) {
        self.broadcast_bot_positions();
        self.broadcast_map_changes();
    }

    fn broadcast_bot_positions(&mut self) {
        use std::time::Instant;

        let now = Instant::now();

        for bot in self.bots.values() {
            let time_since_last_movement = now - bot.last_movement_time;

            if time_since_last_movement.as_secs_f32() > self.config.max_idle_packet_duration {
                continue;
            }

            let area = match self.areas.get(&bot.area_id) {
                Some(area) => area,
                None => continue,
            };

            let packet = ServerPacket::ActorMove {
                actor_id: bot.id.clone(),
                x: bot.x,
                y: bot.y,
                z: bot.z,
                direction: bot.direction,
            };

            broadcast_to_area(
                &mut self.packet_orchestrator.borrow_mut(),
                area,
                Reliability::UnreliableSequenced,
                packet,
            );
        }
    }

    fn broadcast_map_changes(&mut self) {
        use super::asset::get_map_path;

        for area in self.areas.values_mut() {
            let map_path = get_map_path(area.id());
            let map = area.map_mut();

            if map.asset_is_stale() {
                let map_asset = map.generate_asset();

                self.asset_manager.set_asset(map_path.clone(), map_asset);
                update_cached_clients(
                    &mut self.packet_orchestrator.borrow_mut(),
                    self.config.max_payload_size,
                    &self.asset_manager,
                    &mut self.clients,
                    &map_path,
                );

                let packet = ServerPacket::MapUpdate {
                    map_path: map_path.clone(),
                };

                broadcast_to_area(
                    &mut self.packet_orchestrator.borrow_mut(),
                    area,
                    Reliability::ReliableOrdered,
                    packet,
                );
            }
        }
    }
}

fn broadcast_actor_keyframes(
    packet_orchestrator: &mut PacketOrchestrator,
    area: &Area,
    id: &str,
    keyframes: Vec<ActorKeyFrame>,
) {
    packet_orchestrator.broadcast_to_room(
        area.id(),
        Reliability::ReliableOrdered,
        ServerPacket::ActorPropertyKeyFrames {
            actor_id: id.to_string(),
            keyframes,
        },
    );
}

fn update_cached_clients(
    packet_orchestrator: &mut PacketOrchestrator,
    max_payload_size: u16,
    asset_manager: &AssetManager,
    clients: &mut HashMap<String, Client>,
    asset_path: &str,
) {
    use packets::serialize;

    let mut dependencies = asset_manager.get_flattened_dependency_chain(asset_path);
    dependencies.pop();

    let reliability = Reliability::ReliableOrdered;

    let mut clients_to_update: Vec<&mut Client> = clients
        .values_mut()
        .filter(|client| client.cached_assets.contains(asset_path))
        .collect();

    // ensuring dependencies
    for asset_path in dependencies {
        if let Some(asset) = asset_manager.get_asset(asset_path) {
            let mut byte_vecs = Vec::new();

            for client in &mut clients_to_update {
                if client.cached_assets.contains(asset_path) {
                    continue;
                }

                client.cached_assets.insert(asset_path.to_string());

                // lazily create stream
                if byte_vecs.is_empty() {
                    byte_vecs =
                        ServerPacket::create_asset_stream(max_payload_size, asset_path, asset)
                            .map(serialize)
                            .collect();
                }

                packet_orchestrator.send_byte_packets(
                    client.socket_address,
                    reliability,
                    &byte_vecs,
                );
            }
        }
    }

    // updating clients who have this asset
    if let Some(asset) = asset_manager.get_asset(asset_path) {
        let byte_vecs: Vec<Vec<u8>> =
            ServerPacket::create_asset_stream(max_payload_size, asset_path, asset)
                .map(serialize)
                .collect();

        for client in &mut clients_to_update {
            packet_orchestrator.send_byte_packets(client.socket_address, reliability, &byte_vecs);
        }
    }
}

fn broadcast_to_area(
    packet_orchestrator: &mut PacketOrchestrator,
    area: &Area,
    reliability: Reliability,
    packet: ServerPacket,
) {
    packet_orchestrator.broadcast_to_room(area.id(), reliability, packet);
}

fn broadcast_to_scope(
    packet_orchestrator: &mut PacketOrchestrator,
    scope: PacketScope,
    areas: &HashMap<String, Area>,
    reliability: Reliability,
    packet: ServerPacket,
) {
    match scope {
        PacketScope::All => {
            packet_orchestrator.broadcast_to_clients(reliability, packet);
        }
        PacketScope::Area(area_id) => {
            if let Some(area) = areas.get(area_id.as_ref()) {
                broadcast_to_area(packet_orchestrator, area, reliability, packet);
            }
        }
        PacketScope::Client(id) => {
            packet_orchestrator.send_by_id(&id, reliability, packet);
        }
    }
}

fn ensure_asset<I, P>(
    packet_orchestrator: &mut PacketOrchestrator,
    max_payload_size: u16,
    asset_manager: &AssetManager,
    clients: &mut HashMap<String, Client>,
    player_ids: I,
    asset_path: &str,
) where
    I: IntoIterator<Item = P> + Clone,
    P: AsRef<str>,
{
    if !asset_path.starts_with("/server") {
        return;
    }

    let assets_to_send = asset_manager.get_flattened_dependency_chain(asset_path);

    if assets_to_send.is_empty() {
        log::warn!("No asset found with path {:?}", asset_path);
        return;
    }

    for asset_path in assets_to_send {
        let asset = if let Some(asset) = asset_manager.get_asset(asset_path) {
            asset
        } else {
            log::warn!("No asset found with path {:?}", asset_path);
            continue;
        };

        let mut byte_vecs = Vec::new();

        for player_id in player_ids.clone() {
            let client = clients.get_mut(player_id.as_ref()).unwrap();

            if asset.cachable && client.cached_assets.contains(asset_path) {
                continue;
            }

            // lazily create stream
            if byte_vecs.is_empty() {
                use packets::serialize;

                byte_vecs = ServerPacket::create_asset_stream(max_payload_size, asset_path, asset)
                    .map(serialize)
                    .collect();
            }

            if asset.cachable {
                client.cached_assets.insert(asset_path.to_string());
            }

            packet_orchestrator.send_byte_packets(
                client.socket_address,
                Reliability::ReliableOrdered,
                &byte_vecs,
            );
        }
    }
}

fn ensure_assets<AI, A, PI, P>(
    packet_orchestrator: &mut PacketOrchestrator,
    max_payload_size: u16,
    asset_manager: &AssetManager,
    clients: &mut HashMap<String, Client>,
    player_ids: PI,
    asset_paths: AI,
) where
    AI: IntoIterator<Item = A>,
    A: AsRef<str>,
    PI: IntoIterator<Item = P> + Clone,
    P: AsRef<str>,
{
    for asset_path in asset_paths {
        ensure_asset(
            packet_orchestrator,
            max_payload_size,
            asset_manager,
            clients,
            player_ids.clone(),
            asset_path.as_ref(),
        );
    }
}

fn ensure_scope_has_assets<AI, A>(
    packet_orchestrator: &mut PacketOrchestrator,
    max_payload_size: u16,
    asset_manager: &AssetManager,
    areas: &HashMap<String, Area>,
    clients: &mut HashMap<String, Client>,
    scope: &PacketScope,
    asset_paths: AI,
) where
    AI: IntoIterator<Item = A> + Clone,
    A: AsRef<str>,
{
    match scope {
        PacketScope::All => {
            for area in areas.values() {
                ensure_assets(
                    packet_orchestrator,
                    max_payload_size,
                    asset_manager,
                    clients,
                    area.connected_players(),
                    asset_paths.clone(),
                );
            }
        }
        PacketScope::Area(area_id) => {
            if let Some(area) = areas.get(area_id.as_ref()) {
                ensure_assets(
                    packet_orchestrator,
                    max_payload_size,
                    asset_manager,
                    clients,
                    area.connected_players(),
                    asset_paths,
                );
            }
        }
        PacketScope::Client(id) => {
            ensure_assets(
                packet_orchestrator,
                max_payload_size,
                asset_manager,
                clients,
                &[id],
                asset_paths,
            );
        }
    }
}
