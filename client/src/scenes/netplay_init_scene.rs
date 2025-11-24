use super::BattleScene;
use crate::battle::{BattleProps, PlayerSetup};
use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use crate::transitions::{flash_color, HoldColorScene};
use framework::prelude::*;
use futures::Future;
use packets::structures::{FileHash, PackageCategory, PackageId, RemotePlayerInfo};
use packets::{NetplayBufferItem, NetplayPacket, NetplayPacketData, NetplaySignal};
use std::collections::{HashMap, HashSet};
use std::pin::Pin;

const MAX_SILENCE: Duration = Duration::from_secs(3);
const HEARTBEAT_RATE: Duration = Duration::from_secs(1);

pub struct NetplayProps {
    /// Partially configured battle props, should only contain the local player and server sent encounter information
    pub battle_props: BattleProps,
    pub remote_players: Vec<RemotePlayerInfo>,
    pub fallback_address: String,
}

enum Event {
    ResolvedAddresses {
        players: Vec<(NetplayPacketSender, NetplayPacketReceiver)>,
    },
    Fallback {
        fallback: (NetplayPacketSender, NetplayPacketReceiver),
    },
}

#[derive(Default)]
enum ConnectionStage {
    #[default]
    ResolvingAddresses,
    ResolvingSpectators,
    SharingPackageList,
    WaitingToSharePackages,
    SharingPackages,
    Ready,
    HeadingToBattle,
    Failed,
    Complete,
}

impl ConnectionStage {
    fn advance(&mut self) {
        *self = match self {
            ConnectionStage::ResolvingAddresses => ConnectionStage::ResolvingSpectators,
            ConnectionStage::ResolvingSpectators => ConnectionStage::SharingPackageList,
            ConnectionStage::SharingPackageList => ConnectionStage::WaitingToSharePackages,
            ConnectionStage::WaitingToSharePackages => ConnectionStage::SharingPackages,
            ConnectionStage::SharingPackages => ConnectionStage::Ready,
            ConnectionStage::Ready => ConnectionStage::HeadingToBattle,
            ConnectionStage::HeadingToBattle => ConnectionStage::Complete,
            ConnectionStage::Failed => ConnectionStage::Complete,
            ConnectionStage::Complete => ConnectionStage::Complete,
        };
    }
}

struct RemotePlayerConnection {
    last_message: Instant,
    player_setup: PlayerSetup,
    spectating: bool,
    load_map: HashMap<FileHash, PackageCategory>,
    requested_packages: Option<Vec<FileHash>>,
    ready_for_packages: bool,
    received_package_list: bool,
    ready: bool,
    send: Option<NetplayPacketSender>,
    receiver: Option<NetplayPacketReceiver>,
}

pub struct NetplayInitScene {
    stage: ConnectionStage,
    local_index: usize,
    battle_props: Option<BattleProps>,
    last_heartbeat: Instant,
    spectating: bool,
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
            mut battle_props,
            remote_players,
            fallback_address,
        } = props;

        let local_index = Self::resolve_local_index(&remote_players);
        let player_setup = &mut battle_props.meta.player_setups[0];
        player_setup.index = local_index;

        log::debug!("Assigned player index {local_index}");

        let globals = game_io.resource::<Globals>().unwrap();
        let network = &globals.network;

        let remote_index_map: Vec<_> = remote_players.iter().map(|info| info.index).collect();
        let total_remote = remote_players.len();

        // build connection futures
        let mut remote_futures = Vec::with_capacity(total_remote + 1);

        if !globals.config.force_relay && remote_players.iter().all(|r| r.address.is_some()) {
            remote_futures.extend(
                remote_players
                    .iter()
                    .flat_map(|remote_player| remote_player.address)
                    .map(|address| network.subscribe_to_netplay(address.to_string())),
            )
        }

        remote_futures.push(network.subscribe_to_netplay(fallback_address));

        // build hole punching future
        let (event_sender, event_receiver) = flume::unbounded();

        let communication_future = async move {
            let results = futures::future::join_all(remote_futures).await;
            let mut senders_and_receivers: Vec<_> = results.into_iter().flatten().collect();
            let fallback_sender_receiver = senders_and_receivers.pop().unwrap();

            if senders_and_receivers.len() < total_remote {
                log::info!("Using relay due to player preferences");

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

        let now = game_io.frame_start_instant();
        let remote_player_connections: Vec<_> = remote_players
            .into_iter()
            .map(
                |RemotePlayerInfo {
                     address: _,
                     index,
                     health,
                     base_health,
                     emotion,
                 }| RemotePlayerConnection {
                    last_message: now,
                    player_setup: PlayerSetup {
                        health,
                        base_health,
                        emotion,
                        ..PlayerSetup::new_empty(index, false)
                    },
                    spectating: false,
                    load_map: HashMap::new(),
                    requested_packages: None,
                    ready_for_packages: false,
                    received_package_list: false,
                    ready: false,
                    send: None,
                    receiver: None,
                },
            )
            .collect();

        let mut sprite = (globals.assets).new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        sprite.set_color(flash_color(game_io));

        Self {
            stage: Default::default(),
            local_index,
            battle_props: Some(battle_props),
            last_heartbeat: game_io.frame_start_instant(),
            spectating: false,
            missing_packages: HashSet::new(),
            player_connections: remote_player_connections,
            last_fallback_instant: game_io.frame_start_instant(),
            fallback_sender_receiver: None,
            event_receiver,
            ui_camera: Camera::new_ui(game_io),
            sprite,
            communication_future: Some(Box::pin(communication_future)),
            start_instant: Instant::now(),
            next_scene: NextScene::None,
        }
    }

    fn resolve_local_index(remote_players: &[RemotePlayerInfo]) -> usize {
        let mut possible_indexes = Vec::from_iter(0..remote_players.len() + 1);

        for info in remote_players {
            if let Ok(p_index) = possible_indexes.binary_search(&info.index) {
                possible_indexes.remove(p_index);
            }
        }

        possible_indexes.pop().unwrap()
    }

    fn handle_heartbeat(&mut self) {
        let now = Instant::now();

        if now - self.last_heartbeat >= HEARTBEAT_RATE {
            self.last_heartbeat = now;

            self.broadcast(NetplayPacketData::Heartbeat);
        }
    }

    fn handle_packets(&mut self, game_io: &mut GameIO) {
        let mut packets = Vec::new();

        let mut player_disconnected = false;

        if let Some((_, receiver)) = &self.fallback_sender_receiver {
            if receiver.is_disconnected() {
                self.stage = ConnectionStage::Failed;
                return;
            }

            if !receiver.is_empty() {
                self.last_fallback_instant = game_io.frame_start_instant();
            }

            while let Ok(packet) = receiver.try_recv() {
                packets.push(packet);
            }

            if game_io.frame_start_instant() - self.last_fallback_instant > MAX_SILENCE {
                // remote must've disconnected in some edge case, such as leaving before connection even starts
                log::error!("Relayed connection silent");
                self.stage = ConnectionStage::Failed;
            }

            // detect connection loss with other clients
            for connection in &mut self.player_connections {
                if connection.ready {
                    continue;
                }

                if game_io.frame_start_instant() - connection.last_message > MAX_SILENCE {
                    connection.player_setup.connected = false;
                    player_disconnected = true;
                }
            }
        } else {
            for connection in &mut self.player_connections {
                let Some(receiver) = &connection.receiver else {
                    continue;
                };

                while let Ok(packet) = receiver.try_recv() {
                    packets.push(packet);
                }

                if receiver.is_disconnected() {
                    connection.player_setup.connected = false;
                    player_disconnected = true;
                }
            }
        }

        if player_disconnected {
            // identify spectators early if we need to
            self.identify_spectators(game_io);

            // fail if the player that disconnected isn't spectating
            for connection in &self.player_connections {
                if connection.player_setup.connected || connection.spectating {
                    continue;
                }

                // fail entirely if this player is involved in the battle
                self.stage = ConnectionStage::Failed;

                let index = connection.player_setup.index;
                log::error!("Lost connection with player {index}");
            }
        }

        if matches!(self.stage, ConnectionStage::Failed) {
            return;
        }

        for packet in packets {
            self.handle_packet(game_io, packet);
        }
    }

    fn handle_packet(&mut self, game_io: &mut GameIO, packet: NetplayPacket) {
        let index = packet.index;

        let Some(connection) = self
            .player_connections
            .iter_mut()
            .find(|connection| connection.player_setup.index == index)
        else {
            return;
        };

        connection.last_message = game_io.frame_start_instant();

        if !matches!(
            packet.data,
            NetplayPacketData::Buffer { .. } | NetplayPacketData::Heartbeat
        ) {
            let packet_name: &'static str = (&packet.data).into();
            log::debug!("Received {packet_name} from {index}");
        }

        match packet.data {
            NetplayPacketData::Hello => {
                // handled earlier
            }
            NetplayPacketData::HelloAck | NetplayPacketData::Heartbeat => {
                // response unnecessary
            }
            NetplayPacketData::PlayerSetup {
                player_package,
                script_enabled,
                cards,
                regular_card,
                recipes,
                blocks,
                drives,
                input_delay,
                ..
            } => {
                let setup = &mut connection.player_setup;
                setup.package_id = player_package;
                setup.script_enabled = script_enabled;
                setup.deck.cards = cards
                    .into_iter()
                    .map(|(package_id, code)| Card { package_id, code })
                    .collect();
                setup.deck.regular_index = regular_card;
                setup.recipes = recipes;
                setup.blocks = blocks;
                setup.drives = drives;
                setup.buffer.clear();
                setup.buffer.set_delay(input_delay);
            }
            NetplayPacketData::PackageList { packages } => {
                if !connection.spectating {
                    connection.received_package_list = true;

                    let globals = game_io.resource::<Globals>().unwrap();

                    // track what needs to be loaded once downloaded
                    let load_list: Vec<_> = packages
                        .into_iter()
                        .filter(|(category, id, hash)| {
                            globals
                                .package_or_fallback_info(*category, PackageNamespace::Local, id)
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

                    // request missing packages, even if that list is empty
                    // allows other clients to advance to the next connection stage
                    self.send(
                        index,
                        NetplayPacketData::MissingPackages {
                            recipient_index: index,
                            list: missing_packages,
                        },
                    );
                }
            }
            NetplayPacketData::MissingPackages {
                recipient_index,
                list,
            } => {
                if self.local_index == recipient_index {
                    connection.requested_packages = Some(list);
                }
            }
            NetplayPacketData::ReadyForPackages => {
                connection.ready_for_packages = true;
            }
            NetplayPacketData::PackageZip { data } => {
                let hash = FileHash::hash(&data);

                log::debug!("Received zip for {hash}");

                if self.missing_packages.remove(&hash) {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let assets = &globals.assets;
                    assets.load_virtual_zip(game_io, hash, data);

                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    for connection in &mut self.player_connections {
                        if let Some(category) = connection.load_map.remove(&hash) {
                            let namespace =
                                PackageNamespace::Netplay(connection.player_setup.index as u8);
                            let optional_package =
                                globals.load_virtual_package(category, namespace, hash);

                            if let Some(package) = optional_package {
                                log::debug!(
                                    "Loaded {:?} for {}",
                                    package.id,
                                    connection.player_setup.index
                                );
                            }
                        }
                    }
                } else if self.fallback_sender_receiver.is_none() {
                    log::error!("Received data for package that wasn't requested: {hash}");
                    self.stage = ConnectionStage::Failed;
                }
            }
            NetplayPacketData::Ready => {
                connection.ready = true;
                connection.ready_for_packages = true;
            }
            NetplayPacketData::Buffer { data, .. } => {
                self.identify_spectators(game_io);

                let Some(connection) = self
                    .player_connections
                    .iter_mut()
                    .find(|connection| connection.player_setup.index == index)
                else {
                    return;
                };

                if !connection.spectating && data.signals.contains(&NetplaySignal::Disconnect) {
                    self.stage = ConnectionStage::Failed;
                }

                connection.player_setup.buffer.push_last(data);
            }
            NetplayPacketData::ReceiveCounts { .. } => {}
        }
    }

    fn send(&self, remote_index: usize, data: NetplayPacketData) {
        let packet = NetplayPacket {
            index: self.local_index,
            data,
        };

        if let Some((send, _)) = &self.fallback_sender_receiver {
            send(packet);
        } else {
            let connection = self
                .player_connections
                .iter()
                .find(|conn| conn.player_setup.index == remote_index);

            if let Some(connection) = connection {
                if let Some(send) = connection.send.as_ref() {
                    send(packet);
                }
            }
        }
    }

    fn broadcast(&self, data: NetplayPacketData) {
        let packet = NetplayPacket {
            index: self.local_index,
            data,
        };

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

    fn identified_spectators(&self) -> bool {
        self.battle_props
            .as_ref()
            .is_some_and(|p| p.simulation_and_resources.is_some())
    }

    fn identify_spectators(&mut self, game_io: &GameIO) {
        if self.identified_spectators() {
            return;
        }

        let Some(props) = &mut self.battle_props else {
            return;
        };

        props.meta.player_count = self.player_connections.len() + 1;
        props.load_encounter(game_io);

        let Some((_, resources)) = &props.simulation_and_resources else {
            return;
        };

        let config = resources.config.borrow();

        for connection in &mut self.player_connections {
            if config.spectators.contains(&connection.player_setup.index) {
                connection.spectating = true;
                connection.received_package_list = true;
            }
        }

        self.spectating = config.spectators.contains(&self.local_index);
    }

    fn broadcast_package_list(&mut self, game_io: &GameIO) {
        let player_setup = PlayerSetup::from_globals(game_io);
        let globals = game_io.resource::<Globals>().unwrap();

        let packages = globals
            .package_dependency_iter(player_setup.direct_dependency_triplets(game_io))
            .iter()
            .map(|(package_info, _)| {
                (
                    package_info.category,
                    package_info.id.clone(),
                    package_info.hash,
                )
            })
            .collect();

        self.broadcast(NetplayPacketData::PackageList { packages });

        self.broadcast(NetplayPacketData::PlayerSetup {
            player_package: player_setup.package_id,
            script_enabled: player_setup.script_enabled,
            cards: (player_setup.deck.cards.into_iter())
                .map(|card| (card.package_id, card.code))
                .collect(),
            recipes: player_setup.recipes,
            regular_card: player_setup.deck.regular_index,
            blocks: player_setup.blocks,
            drives: player_setup.drives,
            memories: player_setup.memories,
            input_delay: player_setup.buffer.delay(),
        })
    }

    fn share_packages(&mut self, game_io: &GameIO) {
        if self.spectating {
            return;
        }

        let broadcasting = self.fallback_sender_receiver.is_some();
        let mod_cache_folder = ResourcePaths::mod_cache_folder();

        if broadcasting {
            // gathering all of the package requests and merging them to broadcast
            let mut pending_upload = HashSet::new();

            for connection in &mut self.player_connections {
                pending_upload.extend(connection.requested_packages.take().unwrap_or_default());
            }

            for hash in pending_upload {
                let assets = &game_io.resource::<Globals>().unwrap().assets;

                let data = assets.virtual_zip_bytes(&hash).unwrap_or_else(|| {
                    let path = format!("{mod_cache_folder}{hash}.zip");

                    assets.binary(&path)
                });

                self.broadcast(NetplayPacketData::PackageZip { data });
            }

            return;
        }

        // send individually to each client
        for i in 0..self.player_connections.len() {
            let connection = &mut self.player_connections[i];
            let connection_index = connection.player_setup.index;

            for hash in connection.requested_packages.take().unwrap() {
                let assets = &game_io.resource::<Globals>().unwrap().assets;

                let data = assets.virtual_zip_bytes(&hash).unwrap_or_else(|| {
                    let path = format!("{mod_cache_folder}{hash}.zip");

                    assets.binary(&path)
                });

                self.send(connection_index, NetplayPacketData::PackageZip { data });
            }
        }
    }

    fn check_peers(&mut self, f: impl FnMut(&RemotePlayerConnection) -> bool) -> bool {
        self.player_connections
            .iter()
            .filter(|c| c.player_setup.connected)
            .all(f)
    }

    fn finalize_peer_dependency_trees(&self, game_io: &mut GameIO) {
        log::info!("Finalizing peer dependency trees");

        let mut work_list: Vec<PackageId> = Default::default();
        let mut reverse_dependency_map: HashMap<PackageId, Vec<(PackageCategory, PackageId)>> =
            Default::default();

        for connection in self.player_connections.iter() {
            let index = connection.player_setup.index;

            if index == self.local_index {
                continue;
            }

            let player_ns = PackageNamespace::Netplay(index as u8);

            // build worklist
            let globals = game_io.resource::<Globals>().unwrap();
            work_list.extend(globals.packages(player_ns).map(|info| info.id.clone()));

            if work_list.is_empty() {
                // avoid unnecessary work
                continue;
            }

            // resolve dependency map
            let triplet_iter = connection.player_setup.direct_dependency_triplets(game_io);

            for (info, _) in globals.package_dependency_iter(triplet_iter) {
                if info.parent_package.is_some() || info.namespace == player_ns {
                    continue;
                }

                for (_, id) in &info.requirements {
                    let dependents = reverse_dependency_map.entry(id.clone()).or_default();
                    dependents.push((info.category, info.id.clone()));
                }
            }

            // reload anything depending on these mods in the private namespace
            let globals = game_io.resource_mut::<Globals>().unwrap();

            while let Some(id) = work_list.pop() {
                let Some(dependents) = reverse_dependency_map.remove(&id) else {
                    continue;
                };

                for (category, id) in &dependents {
                    log::info!("Reloading {id} in {player_ns:?}");

                    let Some(old_info) = globals.package_or_fallback_info(*category, player_ns, id)
                    else {
                        continue;
                    };

                    if old_info.namespace == player_ns {
                        // already loaded
                        continue;
                    }

                    let base_path = old_info.base_path.clone();
                    let Some(new_info) = globals.load_package(*category, player_ns, &base_path)
                    else {
                        continue;
                    };

                    let child_packages: Vec<_> = new_info.child_packages().collect();

                    globals
                        .character_packages
                        .load_child_packages(player_ns, child_packages);
                }

                work_list.extend(dependents.into_iter().map(|(_, id)| id));
            }

            work_list.clear();
            reverse_dependency_map.clear();
        }
    }

    fn handle_stage(&mut self, game_io: &mut GameIO) {
        match self.stage {
            ConnectionStage::ResolvingAddresses | ConnectionStage::Complete => {}
            ConnectionStage::ResolvingSpectators => {
                // identify spectators when the transition ends to avoid loading hiccups
                // identification might happen early if a player disconnects
                let identified_spectators = self.identified_spectators();

                if identified_spectators || !game_io.is_in_transition() {
                    self.stage.advance();
                    self.identify_spectators(game_io);

                    if !self.spectating {
                        self.broadcast_package_list(game_io);
                    }
                }
            }
            ConnectionStage::SharingPackageList => {
                if self.spectating || self.check_peers(|c| c.requested_packages.is_some()) {
                    self.stage.advance();
                    self.broadcast(NetplayPacketData::ReadyForPackages);
                }
            }
            ConnectionStage::WaitingToSharePackages => {
                if self.check_peers(|c| c.ready_for_packages) {
                    self.stage.advance();
                    self.share_packages(game_io);
                }
            }
            ConnectionStage::SharingPackages => {
                if self.check_peers(|c| c.received_package_list) && self.missing_packages.is_empty()
                {
                    self.stage.advance();
                    self.broadcast(NetplayPacketData::Ready);
                }
            }
            ConnectionStage::Ready => {
                if self.check_peers(|c| c.ready) {
                    self.stage.advance();
                }
            }
            ConnectionStage::HeadingToBattle => {
                if !game_io.is_in_transition() {
                    self.finalize_peer_dependency_trees(game_io);

                    let globals = game_io.resource::<Globals>().unwrap();

                    // clean up zips
                    globals.assets.remove_unused_virtual_zips();

                    let mut props = self.battle_props.take().unwrap();

                    // set up other players
                    for mut connection in std::mem::take(&mut self.player_connections) {
                        let setup = connection.player_setup;
                        let index = setup.index;

                        if !connection.spectating {
                            let namespace = PackageNamespace::Netplay(setup.index as u8);
                            let package = globals
                                .player_packages
                                .package_or_fallback(namespace, &setup.package_id);

                            if package.is_none() {
                                log::error!(
                                    "Never received player package ({}) for player {}",
                                    setup.package_id,
                                    index
                                );
                                self.stage = ConnectionStage::Failed;
                                // put the props back so ConnectionStage::Failed can access the statistics callback
                                self.battle_props = Some(props);
                                return;
                            }
                        }

                        props.meta.player_setups.push(setup);

                        if let Some(send) = connection.send.take() {
                            props.comms.senders.push(send);
                        }

                        if let Some(receiver) = connection.receiver.take() {
                            props.comms.receivers.push((Some(index), receiver));
                        }
                    }

                    if let Some((send, receiver)) = self.fallback_sender_receiver.take() {
                        props.comms.senders.push(send);
                        props.comms.receivers.push((None, receiver));
                    }

                    // create scene
                    let battle_scene = BattleScene::new(game_io, props);
                    let hold_duration = crate::transitions::BATTLE_HOLD_DURATION
                        .checked_sub(self.start_instant.elapsed())
                        .unwrap_or_default();

                    let color = flash_color(game_io);
                    let scene =
                        HoldColorScene::new(game_io, color, hold_duration, move |game_io| {
                            let transition = crate::transitions::new_battle(game_io);
                            NextScene::new_swap(battle_scene).with_transition(transition)
                        });

                    let transition = crate::transitions::new_battle(game_io);
                    self.next_scene = NextScene::new_swap(scene).with_transition(transition);
                    self.stage.advance();
                }
            }
            ConnectionStage::Failed => {
                if !game_io.is_in_transition() {
                    // let other players know we're giving up on them
                    self.broadcast(NetplayPacketData::Buffer {
                        data: NetplayBufferItem {
                            pressed: Vec::new(),
                            signals: vec![NetplaySignal::Disconnect],
                        },
                    });

                    // make sure the statistics callback gets called
                    if let Some(props) = self.battle_props.take() {
                        if let Some(callback) = props.statistics_callback {
                            callback(None);
                        }
                    }

                    // move on
                    let transition = crate::transitions::new_battle_pop(game_io);
                    self.next_scene = NextScene::new_pop().with_transition(transition);
                    self.stage.advance();
                }
            }
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

        let loaded_namespaces = globals
            .namespaces()
            .filter(|ns| ns.is_netplay())
            .collect::<Vec<_>>();

        for namespace in loaded_namespaces {
            globals.remove_namespace(namespace);
        }

        if let Some(future) = self.communication_future.take() {
            game_io.spawn_local_task(future).detach();
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::ResolvedAddresses { players } => {
                    for (i, (send, receiver)) in players.into_iter().enumerate() {
                        let connection = &mut self.player_connections[i];
                        connection.send = Some(send);
                        connection.receiver = Some(receiver);
                    }

                    self.stage.advance();
                }
                Event::Fallback { fallback } => {
                    self.last_fallback_instant = game_io.frame_start_instant();
                    self.fallback_sender_receiver = Some(fallback);
                    self.stage.advance();
                }
            }
        }

        if !matches!(self.stage, ConnectionStage::Failed) {
            self.handle_heartbeat();
            self.handle_packets(game_io);
        }

        self.handle_stage(game_io);
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
        send(NetplayPacket {
            index: local_index,
            data: NetplayPacketData::Hello,
        });
    }

    // expecting the first message from everyone to be Hello and the second is a HelloAck
    let hello_streams = senders_and_receivers.iter().map(|(_, receiver)| {
        Box::pin(
            receiver
                .stream()
                .skip_while(|packet| {
                    // skip non hello packets as those are leftovers from a previous match
                    let out = !matches!(packet.data, NetplayPacketData::Hello);
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
        let out = !matches!(packet.data, NetplayPacketData::Hello);
        async move { out }
    });

    // a short amount of time for responses
    let timer = async_sleep(Duration::from_secs(2)).fuse();
    futures::pin_mut!(fallback_stream, timer);

    loop {
        futures::select! {
            packet = hello_stream.select_next_some() => {
                match packet.data {
                    NetplayPacketData::Hello => {
                        log::debug!("Received Hello");

                        if let Some(slice_index) = remote_index_map.iter().position(|i| *i == packet.index) {
                            log::debug!("Sending HelloAck");

                            let send = &senders_and_receivers[slice_index].0;
                            send(NetplayPacket {
                                index: local_index,
                                data: NetplayPacketData::HelloAck
                            });
                        }
                    }
                    NetplayPacketData::HelloAck => {
                        log::debug!("Received HelloAck");

                        total_responses += 1;

                        if total_responses == senders_and_receivers.len() {
                            // received a response from everyone, looks like we all support hole punching
                            return true;
                        }
                    }
                    _ => {
                        let name: &'static str = (&packet.data).into();
                        let index = packet.index;

                        log::error!("Expecting Hello or HelloAck during hole punching phase, received: {name} from {index}");
                    }
                }
            }
            packet = fallback_stream.select_next_some() => {
                if let NetplayPacketData::Hello = packet.data {
                    log::debug!("Received Hello through fallback channel");
                    return false;
                }
            }
            _ = timer => {
                log::debug!("Hole punch timer exhausted");

                // out of time, we'll assume hole punching failed

                // send a hello message on the fallback channel to force wait timers on other players to end
                fallback_sender_receiver.0(NetplayPacket{
                    index: local_index,
                    data: NetplayPacketData::Hello });

                return false;
            }
        }
    }
}
