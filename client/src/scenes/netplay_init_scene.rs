use super::BattleScene;
use crate::battle::{BattleProps, BattleStatisticsCallback, PlayerSetup};
use crate::bindable::SpriteColorMode;
use crate::packages::{PackageId, PackageNamespace};
use crate::render::*;
use crate::resources::*;
use crate::saves::{Card, Deck};
use crate::transitions::HoldColorScene;
use framework::prelude::*;
use futures::Future;
use packets::structures::{Emotion, FileHash, InstalledBlock, PackageCategory, RemotePlayerInfo};
use packets::{NetplayBufferItem, NetplayPacket, NetplaySignal, SERVER_TICK_RATE};
use rand::rngs::OsRng;
use rand::RngCore;
use std::collections::{HashMap, HashSet, VecDeque};
use std::pin::Pin;

const MAX_FALLBACK_SILENCE: Duration = Duration::from_secs(3);

pub struct NetplayProps {
    pub background: Option<Background>,
    pub encounter_package: Option<(PackageNamespace, PackageId)>,
    pub data: Option<String>,
    pub health: i32,
    pub base_health: i32,
    pub emotion: Emotion,
    pub remote_players: Vec<RemotePlayerInfo>,
    pub fallback_address: String,
    pub statistics_callback: Option<BattleStatisticsCallback>,
}

enum Event {
    AddressesFailed,
    ResolvedAddresses {
        players: Vec<(NetplayPacketSender, NetplayPacketReceiver)>,
    },
    Fallback {
        fallback: (NetplayPacketSender, NetplayPacketReceiver),
    },
}

struct RemotePlayerConnection {
    index: usize,
    player_package: PackageId,
    script_enabled: bool,
    health: i32,
    base_health: i32,
    emotion: Emotion,
    deck: Deck,
    blocks: Vec<InstalledBlock>,
    load_map: HashMap<FileHash, PackageCategory>,
    requested_packages: Option<Vec<FileHash>>,
    ready_for_packages: bool,
    received_package_list: bool,
    ready: bool,
    send: Option<NetplayPacketSender>,
    receiver: Option<NetplayPacketReceiver>,
    buffer: VecDeque<NetplayBufferItem>,
}

pub struct NetplayInitScene {
    local_index: usize,
    local_health: i32,
    local_base_health: i32,
    local_emotion: Emotion,
    encounter_package: Option<(PackageNamespace, PackageId)>,
    data: Option<String>,
    background: Option<Background>,
    statistics_callback: Option<BattleStatisticsCallback>,
    last_heartbeat: Instant,
    failed: bool,
    seed: u64,
    missing_packages: HashSet<FileHash>,
    player_connections: Vec<RemotePlayerConnection>,
    last_fallback_instant: Instant,
    fallback_sender_receiver: Option<(NetplayPacketSender, NetplayPacketReceiver)>,
    event_receiver: flume::Receiver<Event>,
    ui_camera: Camera,
    sprite: Sprite,
    communication_future: Option<Pin<Box<dyn Future<Output = ()>>>>,
    start_instant: Instant,
    next_scene: NextScene,
}

impl NetplayInitScene {
    pub fn new(game_io: &GameIO, props: NetplayProps) -> Self {
        let NetplayProps {
            background,
            encounter_package,
            data,
            health,
            base_health,
            emotion,
            remote_players,
            fallback_address,
            statistics_callback,
        } = props;

        let local_index = Self::resolve_local_index(&remote_players);

        log::debug!("Assigned player index {}", local_index);

        let globals = game_io.resource::<Globals>().unwrap();
        let network = &globals.network;

        let remote_index_map: Vec<_> = remote_players.iter().map(|info| info.index).collect();
        let total_remote = remote_players.len();

        let remote_futures: Vec<_> = remote_players
            .iter()
            .map(|remote_player| network.subscribe_to_netplay(remote_player.address.to_string()))
            .chain(std::iter::once(
                network.subscribe_to_netplay(fallback_address),
            ))
            .collect();

        let (event_sender, event_receiver) = flume::unbounded();

        let communication_future = async move {
            let results = futures::future::join_all(remote_futures).await;
            let mut senders_and_receivers: Vec<_> = results.into_iter().flatten().collect();
            let fallback_sender_receiver = senders_and_receivers.pop().unwrap();

            if senders_and_receivers.len() < total_remote {
                log::error!("Server sent an invalid address for a remote player, using fallback");

                let _ = event_sender.send(Event::Fallback {
                    fallback: fallback_sender_receiver,
                });
                return;
            }

            let success = punch_holes(
                local_index,
                &remote_index_map,
                &senders_and_receivers,
                &fallback_sender_receiver,
            )
            .await;

            let event = if success {
                log::debug!("Hole punching successful");
                Event::ResolvedAddresses {
                    players: senders_and_receivers,
                }
            } else {
                log::debug!("Hole punching failed");
                Event::Fallback {
                    fallback: fallback_sender_receiver,
                }
            };

            let _ = event_sender.send(event);
        };

        let remote_player_connections: Vec<_> = remote_players
            .into_iter()
            .map(|info| RemotePlayerConnection {
                index: info.index,
                player_package: PackageId::new_blank(),
                script_enabled: true,
                health: info.health,
                base_health: info.base_health,
                emotion: info.emotion,
                deck: Deck::default(),
                blocks: Vec::new(),
                load_map: HashMap::new(),
                requested_packages: None,
                ready_for_packages: false,
                received_package_list: false,
                ready: false,
                send: None,
                receiver: None,
                buffer: VecDeque::new(),
            })
            .collect();

        Self {
            local_index,
            local_health: health,
            local_base_health: base_health,
            local_emotion: emotion,
            encounter_package,
            data,
            background,
            statistics_callback,
            last_heartbeat: game_io.frame_start_instant(),
            failed: false,
            seed: 0,
            missing_packages: HashSet::new(),
            player_connections: remote_player_connections,
            last_fallback_instant: game_io.frame_start_instant(),
            fallback_sender_receiver: None,
            event_receiver,
            ui_camera: Camera::new_ui(game_io),
            sprite: (globals.assets).new_sprite(game_io, ResourcePaths::WHITE_PIXEL),
            communication_future: Some(Box::pin(communication_future)),
            start_instant: Instant::now(),
            next_scene: NextScene::None,
        }
    }

    fn resolve_local_index(remote_players: &[RemotePlayerInfo]) -> usize {
        let mut possible_indexes = Vec::from_iter(0..remote_players.len() + 1);

        for info in remote_players {
            if let Some(p_index) = possible_indexes
                .iter()
                .position(|index| *index == info.index)
            {
                possible_indexes.remove(p_index);
            }
        }

        possible_indexes.pop().unwrap()
    }

    fn handle_heartbeat(&mut self) {
        let now = Instant::now();

        if now - self.last_heartbeat >= SERVER_TICK_RATE {
            self.last_heartbeat = now;

            self.broadcast(NetplayPacket::Heartbeat {
                index: self.local_index,
            });
        }
    }

    fn handle_packets(&mut self, game_io: &mut GameIO) {
        let mut packets = Vec::new();

        if let Some((_, receiver)) = &self.fallback_sender_receiver {
            if receiver.is_disconnected() {
                self.failed = true;
                return;
            }

            if !receiver.is_empty() {
                self.last_fallback_instant = game_io.frame_start_instant();
            }

            while let Ok(packet) = receiver.try_recv() {
                packets.push(packet);
            }

            if game_io.frame_start_instant() - self.last_fallback_instant > MAX_FALLBACK_SILENCE {
                // remote must've disconnected in some edge case, such as leaving before connection even starts
                self.failed = true;
            }
        } else {
            for connection in &self.player_connections {
                if let Some(receiver) = &connection.receiver {
                    if receiver.is_disconnected() {
                        self.failed = true;
                        return;
                    }

                    while let Ok(packet) = receiver.try_recv() {
                        packets.push(packet);
                    }
                }
            }
        }

        if self.failed {
            return;
        }

        for packet in packets {
            self.handle_packet(game_io, packet);
        }
    }

    fn handle_packet(&mut self, game_io: &mut GameIO, packet: NetplayPacket) {
        let index = packet.index();

        let connection = self
            .player_connections
            .iter_mut()
            .find(|connection| connection.index == index);

        let connection = match connection {
            Some(connection) => connection,
            None => return,
        };

        if !matches!(
            packet,
            NetplayPacket::Buffer { .. } | NetplayPacket::Heartbeat { .. }
        ) {
            let packet_name: &'static str = (&packet).into();
            log::debug!("Received {packet_name} from {index}");
        }

        match packet {
            NetplayPacket::Hello { .. } => {
                // handled earlier
            }
            NetplayPacket::HelloAck { .. } | NetplayPacket::Heartbeat { .. } => {
                // response unnecessary
            }
            NetplayPacket::PlayerSetup {
                player_package,
                script_enabled,
                cards,
                regular_card,
                blocks,
                ..
            } => {
                connection.player_package = player_package;
                connection.script_enabled = script_enabled;
                connection.deck.regular_index = regular_card;
                connection.deck.cards = cards
                    .into_iter()
                    .map(|(package_id, code)| Card { package_id, code })
                    .collect();
                connection.blocks = blocks;
            }
            NetplayPacket::PackageList { index, packages } => {
                connection.received_package_list = true;

                let globals = game_io.resource::<Globals>().unwrap();

                // track what needs to be loaded once downloaded
                let load_list: Vec<_> = packages
                    .into_iter()
                    .filter(|(category, id, hash)| {
                        globals
                            .package_or_override_info(*category, PackageNamespace::Local, id)
                            .map(|package_info| package_info.hash != *hash) // hashes differ
                            .unwrap_or(true) // non existent
                    })
                    .collect();

                for (category, _, hash) in &load_list {
                    connection.load_map.insert(*hash, *category);
                }

                // track files we need to download
                let missing_packages: Vec<_> = load_list
                    .into_iter()
                    .map(|(_, _, hash)| hash)
                    .filter(|hash| self.missing_packages.insert(*hash))
                    .collect();

                if missing_packages.is_empty() && self.received_every_zip() {
                    self.broadcast_ready();
                }

                // request missing packages, even if that list is empty
                // check next block to see why
                self.send(
                    index,
                    NetplayPacket::MissingPackages {
                        index: self.local_index,
                        recipient_index: index,
                        list: missing_packages,
                    },
                );
            }
            NetplayPacket::MissingPackages {
                recipient_index,
                list,
                ..
            } => {
                if self.local_index == recipient_index {
                    connection.requested_packages = Some(list);

                    if self.received_every_missing_list() {
                        self.broadcast(NetplayPacket::ReadyForPackages {
                            index: self.local_index,
                        });
                    }
                }
            }
            NetplayPacket::ReadyForPackages { .. } => {
                connection.ready_for_packages = true;

                if self.all_ready_for_packages() {
                    self.share_packages(game_io);
                }
            }
            NetplayPacket::PackageZip { data, .. } => {
                let hash = FileHash::hash(&data);

                log::debug!("Received zip for {hash}");

                if self.missing_packages.remove(&hash) {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let assets = &globals.assets;
                    assets.load_virtual_zip(game_io, hash, data);

                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    for connection in &mut self.player_connections {
                        if let Some(category) = connection.load_map.remove(&hash) {
                            let namespace = PackageNamespace::Netplay(connection.index);
                            globals.load_virtual_package(category, namespace, hash);
                        }
                    }

                    if self.received_every_zip() {
                        self.broadcast_ready();
                    }
                } else if self.fallback_sender_receiver.is_none() {
                    log::error!("Received data for package that wasn't requested: {hash}");
                    self.failed = true;
                }
            }
            NetplayPacket::Ready { seed, .. } => {
                // todo: prevent seed manipulation
                self.seed = self.seed.max(seed);
                connection.ready = true;
            }
            NetplayPacket::Buffer { data, .. } => {
                if data.signals.contains(&NetplaySignal::Disconnect) {
                    self.failed = true;
                }

                connection.buffer.push_back(data);
            }
        }
    }

    fn all_ready(&self) -> bool {
        self.received_every_zip()
            && self
                .player_connections
                .iter()
                .all(|connection| connection.ready)
    }

    fn received_every_missing_list(&self) -> bool {
        self.player_connections
            .iter()
            .all(|connection| connection.requested_packages.is_some())
    }

    fn all_ready_for_packages(&self) -> bool {
        self.player_connections
            .iter()
            .all(|connection| connection.ready_for_packages)
    }

    fn received_every_package_list(&self) -> bool {
        self.player_connections
            .iter()
            .all(|connection| connection.received_package_list)
    }

    fn received_every_zip(&self) -> bool {
        self.missing_packages.is_empty() && self.received_every_package_list()
    }

    fn send(&self, remote_index: usize, packet: NetplayPacket) {
        if let Some((send, _)) = &self.fallback_sender_receiver {
            send(packet);
        } else {
            let connection = self
                .player_connections
                .iter()
                .find(|conn| conn.index == remote_index);

            if let Some(connection) = connection {
                if let Some(send) = connection.send.as_ref() {
                    send(packet);
                }
            }
        }
    }

    fn broadcast(&self, packet: NetplayPacket) {
        if let Some((send, _)) = &self.fallback_sender_receiver {
            send(packet);
        } else {
            for connection in &self.player_connections {
                if let Some(send) = connection.send.as_ref() {
                    send(packet.clone());
                }
            }
        }
    }

    fn broadcast_package_list(&self, game_io: &GameIO) {
        let props = BattleProps::new_with_defaults(game_io, None);
        let globals = game_io.resource::<Globals>().unwrap();
        let dependencies = globals.battle_dependencies(game_io, &props);

        let packages = dependencies
            .iter()
            .map(|(package_info, _)| {
                (
                    package_info.package_category,
                    package_info.id.clone(),
                    package_info.hash,
                )
            })
            .collect();

        self.broadcast(NetplayPacket::PackageList {
            index: self.local_index,
            packages,
        });

        let player_setup = &props.player_setups[0];
        let player_package_info = &player_setup.player_package.package_info;
        let cards = (player_setup.deck.cards.iter())
            .map(|card| (card.package_id.clone(), card.code.clone()))
            .collect();
        let blocks = player_setup.blocks.clone();

        self.broadcast(NetplayPacket::PlayerSetup {
            index: self.local_index,
            player_package: player_package_info.id.clone(),
            script_enabled: player_setup.script_enabled,
            cards,
            regular_card: player_setup.deck.regular_index,
            blocks,
        })
    }

    fn share_packages(&mut self, game_io: &GameIO) {
        let broadcasting = self.fallback_sender_receiver.is_some();

        if broadcasting {
            // gathering all of the package requests and merging them to broadcast
            let mut pending_upload = HashSet::new();

            for connection in &mut self.player_connections {
                pending_upload.extend(connection.requested_packages.take().unwrap_or_default());
            }

            for hash in pending_upload {
                let assets = &game_io.resource::<Globals>().unwrap().assets;

                let data = if let Some(bytes) = assets.virtual_zip_bytes(&hash) {
                    bytes
                } else {
                    let path = format!("{}{}.zip", ResourcePaths::MOD_CACHE_FOLDER, hash);

                    assets.binary(&path)
                };

                self.broadcast(NetplayPacket::PackageZip {
                    index: self.local_index,
                    data,
                });
            }

            return;
        }

        // send individually to each client
        for i in 0..self.player_connections.len() {
            let connection = &mut self.player_connections[i];
            let connection_index = connection.index;

            for hash in connection.requested_packages.take().unwrap() {
                let assets = &game_io.resource::<Globals>().unwrap().assets;

                let data = if let Some(bytes) = assets.virtual_zip_bytes(&hash) {
                    bytes
                } else {
                    let path = format!("{}{}.zip", ResourcePaths::MOD_CACHE_FOLDER, hash);

                    assets.binary(&path)
                };

                self.send(
                    connection_index,
                    NetplayPacket::PackageZip {
                        index: self.local_index,
                        data,
                    },
                );
            }
        }
    }

    fn broadcast_ready(&mut self) {
        let seed = OsRng.next_u64();

        self.broadcast(NetplayPacket::Ready {
            index: self.local_index,
            seed,
        });

        if self.seed < seed {
            self.seed = seed;
        }
    }

    fn handle_transition(&mut self, game_io: &GameIO) {
        if self.failed {
            // let other player's know we're giving up on them
            self.broadcast(NetplayPacket::Buffer {
                index: self.local_index,
                data: NetplayBufferItem {
                    pressed: Vec::new(),
                    signals: vec![NetplaySignal::Disconnect],
                },
                buffer_sizes: Vec::new(),
            });

            // make sure the statistics callback gets called
            if let Some(callback) = self.statistics_callback.take() {
                callback(None);
            }

            // move on
            let transition = crate::transitions::new_battle_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
            return;
        }

        if self.all_ready() {
            let globals = game_io.resource::<Globals>().unwrap();

            // clean up zips
            globals.assets.remove_unused_virtual_zips();

            // get package
            let encounter_package = self.encounter_package.take();
            let encounter_package = encounter_package.and_then(|(namespace, package_id)| {
                globals
                    .encounter_packages
                    .package_or_override(namespace, &package_id)
            });

            let mut props = BattleProps::new_with_defaults(game_io, encounter_package);

            props.statistics_callback = self.statistics_callback.take();
            props.data = self.data.take();
            props.seed = Some(self.seed);

            // copy background
            if let Some(background) = self.background.take() {
                props.background = background;
            }

            // correct index and health
            let local_setup = &mut props.player_setups[0];
            local_setup.index = self.local_index;
            local_setup.health = self.local_health;
            local_setup.base_health = self.local_base_health;
            local_setup.emotion = self.local_emotion.clone();

            // setup other players
            for mut connection in std::mem::take(&mut self.player_connections) {
                let namespace = PackageNamespace::Netplay(connection.index);
                let package = globals
                    .player_packages
                    .package_or_override(namespace, &connection.player_package);

                let player_package = match package {
                    Some(package) => package,
                    None => {
                        log::error!(
                            "Never received player package for player {}",
                            connection.index
                        );
                        self.failed = true;
                        return;
                    }
                };

                props.player_setups.push(PlayerSetup {
                    player_package,
                    script_enabled: connection.script_enabled,
                    health: connection.health,
                    base_health: connection.base_health,
                    emotion: connection.emotion,
                    deck: connection.deck.clone(),
                    blocks: connection.blocks.clone(),
                    index: connection.index,
                    local: false,
                    buffer: connection.buffer,
                });

                if let Some(send) = connection.send.take() {
                    props.senders.push(send);
                }

                if let Some(receiver) = connection.receiver.take() {
                    props.receivers.push((Some(connection.index), receiver));
                }
            }

            if let Some((send, receiver)) = self.fallback_sender_receiver.take() {
                props.senders.push(send);
                props.receivers.push((None, receiver));
            }

            // create scene
            let battle_scene = BattleScene::new(game_io, props);
            let hold_duration = crate::transitions::BATTLE_HOLD_DURATION
                .checked_sub(self.start_instant.elapsed())
                .unwrap_or_default();

            let scene = HoldColorScene::new(game_io, Color::WHITE, hold_duration, move |game_io| {
                let transition = crate::transitions::new_battle(game_io);
                NextScene::new_swap(battle_scene).with_transition(transition)
            });

            let transition = crate::transitions::new_battle(game_io);
            self.next_scene = NextScene::new_swap(scene).with_transition(transition);
        }
    }
}

impl Scene for NetplayInitScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        let globals = game_io.resource_mut::<Globals>().unwrap();

        globals.audio.stop_music();
        globals.audio.play_sound(&globals.sfx.battle_transition);

        for namespace in globals.netplay_namespaces() {
            globals.remove_namespace(namespace);
        }

        if let Some(future) = self.communication_future.take() {
            game_io.spawn_local_task(future).detach();
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::AddressesFailed => {
                    self.failed = true;
                }
                Event::ResolvedAddresses { players } => {
                    for (i, (send, receiver)) in players.into_iter().enumerate() {
                        let connection = &mut self.player_connections[i];
                        connection.send = Some(send);
                        connection.receiver = Some(receiver);
                    }

                    self.broadcast_package_list(game_io);
                }
                Event::Fallback { fallback } => {
                    self.last_fallback_instant = game_io.frame_start_instant();
                    self.fallback_sender_receiver = Some(fallback);
                    self.broadcast_package_list(game_io);
                }
            }
        }

        if !game_io.is_in_transition() {
            self.handle_transition(game_io);
        }

        if !self.failed {
            self.handle_heartbeat();
            self.handle_packets(game_io);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.ui_camera, SpriteColorMode::Multiply);

        self.sprite.set_size(RESOLUTION_F);
        sprite_queue.draw_sprite(&self.sprite);

        render_pass.consume_queue(sprite_queue);
    }
}

async fn punch_holes(
    local_index: usize,
    remote_index_map: &[usize],
    senders_and_receivers: &[(NetplayPacketSender, NetplayPacketReceiver)],
    fallback_sender_receiver: &(NetplayPacketSender, NetplayPacketReceiver),
) -> bool {
    use futures::future::FutureExt;
    use futures::StreamExt;

    for (send, _) in senders_and_receivers {
        send(NetplayPacket::Hello { index: local_index });
    }

    // expecting the first message from everyone to be Hello and the second is a HelloAck
    let hello_streams = senders_and_receivers.iter().map(|(_, receiver)| {
        Box::pin(
            receiver
                .stream()
                .skip_while(|packet| {
                    // skip non hello packets as those are leftovers from a previous match
                    let out = !matches!(packet, NetplayPacket::Hello { .. });
                    async move { out }
                })
                .take(2),
        )
    });

    let mut hello_stream = futures::stream::select_all(hello_streams);
    let mut total_responses = 0;

    // if we receive anything from the fallback future we'll switch to it
    let fallback_stream = fallback_sender_receiver.1.stream().skip_while(|packet| {
        // skip non hello packets as those are leftovers from a previous match
        let out = !matches!(packet, NetplayPacket::Hello { .. });
        async move { out }
    });

    // a short amount of time for responses
    let timer = async_sleep(Duration::from_secs(2)).fuse();
    futures::pin_mut!(fallback_stream, timer);

    loop {
        futures::select! {
            result = hello_stream.select_next_some() => {
                match result {
                    NetplayPacket::Hello { index } => {
                        log::debug!("Received Hello");

                        if let Some(slice_index) = remote_index_map.iter().position(|i| *i == index) {
                            log::debug!("Sending HelloAck");

                            let send = &senders_and_receivers[slice_index].0;
                            send(NetplayPacket::HelloAck { index: local_index });
                        }
                    }
                    NetplayPacket::HelloAck {..} => {
                        log::debug!("Received HelloAck");

                        total_responses += 1;

                        if total_responses == senders_and_receivers.len() {
                            // received a response from everyone, looks like we all support hole punching
                            return true;
                        }
                    }
                    packet => {
                        let name: &'static str = (&packet).into();
                        let index = packet.index();

                        log::error!("Expecting Hello or HelloAck during hole punching phase, received: {name} from {index}");
                    }
                }
            }
            result = fallback_stream.select_next_some() => {
                if let NetplayPacket::Hello { .. } = result {
                    log::debug!("Received Hello through fallback channel");
                    return false;
                }
            }
            _ = timer => {
                log::debug!("Hole punch timer exhausted");

                // out of time, we'll assume hole punching failed

                // send a hello message on the fallback channel to force wait timers on other players to end
                fallback_sender_receiver.0(NetplayPacket::Hello {
                    index: local_index,
                });

                return false;
            }
        }
    }
}
