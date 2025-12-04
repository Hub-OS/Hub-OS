use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::{
    FontName, SceneTitle, ScrollableList, SubSceneFrame, Text, TextStyle, Textbox,
    TextboxDoorstopKey, TextboxMessage, TextboxQuestion, UiButton, UiInputTracker, UiNode,
};
use crate::render::{Animator, Background, Camera, SpriteColorQueue};
use crate::resources::{
    AssetManager, Globals, Input, InputUtil, MulticastReceiver, ResourcePaths,
    SyncDataPacketReceiver, SyncDataPacketSender, TEXT_DARK_SHADOW_COLOR,
};
use crate::saves::GlobalSave;
use crate::structures::VecMap;
use framework::math::Vec2;
use framework::prelude::{GameIO, NextScene, Rect, RenderPass, Scene};
use packets::address_parsing::{uri_decode, uri_encode};
use packets::structures::{FileHash, PackageCategory, PackageId, Uuid};
use packets::{MulticastPacket, Reliability, SyncDataPacket, deserialize, serialize};
use std::net::SocketAddr;
use std::time::{Duration, Instant};

const BROADCAST_RATE: Duration = Duration::from_secs(1);

enum Event {
    RequestSyncWith { address: SocketAddr, uuid: Uuid },
    AcceptSyncWith,
    RejectSyncWith,
    SyncConnectionResolved(Option<(SyncDataPacketSender, SyncDataPacketReceiver)>),
}

pub struct SyncDataScene {
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
    frame: SubSceneFrame,
    local_address_position: Vec2,
    ui_input_tracker: UiInputTracker,
    list: ScrollableList,
    discovered: VecMap<SocketAddr, (Uuid, String)>,
    multicast_receiver: MulticastReceiver,
    last_broadcast: Instant,
    requesting_connection_with: Option<(SocketAddr, Uuid)>,
    sync_comms: Option<(SyncDataPacketSender, SyncDataPacketReceiver)>,
    accepted_sync: bool,
    started_connection: bool,
    // pending request
    pending_packages: Vec<(PackageCategory, PackageId)>,
    total_packages: usize,
    // pending send
    pending_identity: Vec<(String, u64)>,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    textbox: Textbox,
    doorstop_key: Option<TextboxDoorstopKey>,
    next_scene: NextScene,
}

impl SyncDataScene {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // layout
        let mut ui_animator = Animator::load_new(assets, ResourcePaths::SYNC_DATA_UI_ANIMATION);
        ui_animator.set_state("DEFAULT");

        let list_bounds = Rect::from_corners(
            ui_animator.point_or_zero("LIST_START"),
            ui_animator.point_or_zero("LIST_END"),
        );

        let (event_sender, event_receiver) = flume::unbounded();

        let discovered = Default::default();

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            scene_title: SceneTitle::new(game_io, "sync-data-scene-title"),
            frame: SubSceneFrame::new(game_io)
                .with_top_bar(true)
                .with_arms(true),
            local_address_position: ui_animator.point_or_zero("LOCAL_ADDRESS"),
            ui_input_tracker: UiInputTracker::new(),
            list: ScrollableList::new(game_io, list_bounds, 15.0).with_children(
                Self::generate_list_children(game_io, &event_sender, &discovered),
            ),
            discovered,
            multicast_receiver: globals.network.subscribe_to_multicast(),
            last_broadcast: Instant::now(),
            requesting_connection_with: None,
            sync_comms: None,
            accepted_sync: false,
            started_connection: false,
            pending_packages: Default::default(),
            total_packages: 0,
            pending_identity: Default::default(),
            event_sender,
            event_receiver,
            textbox: Textbox::new_navigation(game_io),
            doorstop_key: None,
            next_scene: NextScene::None,
        }
    }

    fn generate_list_children(
        game_io: &GameIO,
        event_sender: &flume::Sender<Event>,
        discovered: &VecMap<SocketAddr, (Uuid, String)>,
    ) -> Vec<Box<dyn UiNode>> {
        if discovered.is_empty() {
            return vec![Box::new(
                Text::new(game_io, FontName::Thick).with_str("Scanning for peers..."),
            )];
        }

        struct Row {
            address: String,
            name: String,
        }

        impl UiNode for Row {
            fn draw_bounded(
                &mut self,
                game_io: &GameIO,
                sprite_queue: &mut SpriteColorQueue,
                bounds: Rect,
            ) {
                let mut text_style = TextStyle::new(game_io, FontName::Thick);
                text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
                text_style.bounds = bounds;

                // draw address
                let address_metrics = text_style.measure(&self.address);
                text_style.bounds.x = bounds.right() - address_metrics.size.x;
                text_style.draw(game_io, sprite_queue, &self.address);

                // draw name
                text_style.ellipsis = true;
                text_style.bounds.x = bounds.x;
                text_style.bounds.width -= address_metrics.size.x + text_style.measure(" ").size.x;
                text_style.draw(game_io, sprite_queue, &self.name);
            }

            fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
                Vec2::ZERO
            }
        }

        discovered
            .iter()
            .map(|(address, (uuid, name))| -> Box<dyn UiNode> {
                let uuid = *uuid;
                let row = Row {
                    name: name.clone(),
                    address: address.to_string(),
                };

                Box::new(UiButton::new(row).on_activate({
                    let event_sender = event_sender.clone();
                    let address = *address;

                    move || {
                        let event = Event::RequestSyncWith { address, uuid };
                        event_sender.send(event).unwrap();
                    }
                }))
            })
            .collect()
    }

    fn send(&mut self, game_io: &GameIO, packet: SyncDataPacket) {
        let Some((send, _)) = &self.sync_comms else {
            return;
        };

        // update textbox based on outgoing packets
        let globals = game_io.resource::<Globals>().unwrap();

        match packet {
            SyncDataPacket::PackageList { .. } => {
                let message = globals.translate("sync-data-syncing-packages-message");
                let key = self.textbox.push_doorstop_with_message(message);
                self.doorstop_key = Some(key);
            }
            SyncDataPacket::RequestSave => {
                let message = globals.translate("sync-data-syncing-save-message");
                let key = self.textbox.push_doorstop_with_message(message);
                self.doorstop_key = Some(key);
            }
            SyncDataPacket::Complete { cancelled } => {
                self.doorstop_key.take();

                let message_key = if cancelled {
                    "sync-data-cancelled-message"
                } else {
                    "sync-data-complete-message"
                };

                // update avatar to match the latest save
                self.textbox.use_navigation_avatar(game_io);

                let message = globals.translate(message_key);
                let interface = TextboxMessage::new(message);
                self.textbox.push_interface(interface);
            }
            _ => {}
        }

        send(Reliability::ReliableOrdered, packet);
    }

    fn start_connection(&mut self, game_io: &GameIO, addr: SocketAddr) {
        self.sync_comms = None;

        let globals = game_io.resource::<Globals>().unwrap();
        let subscription_future = globals.network.subscribe_to_sync_data(addr.to_string());

        let sender = self.event_sender.clone();

        game_io
            .spawn_local_task(async move {
                let result = subscription_future.await;
                let event = Event::SyncConnectionResolved(result);
                let _ = sender.send(event);
            })
            .detach();
    }

    fn send_package_list(&mut self, game_io: &GameIO) {
        log::debug!("Sending package list");

        let globals = game_io.resource::<Globals>().unwrap();
        let list = globals
            .packages(PackageNamespace::Local)
            .filter(|info| info.shareable && info.hash != FileHash::ZERO)
            .map(|info| (info.category, info.id.clone()))
            .collect();
        self.send(game_io, SyncDataPacket::PackageList { list });
    }

    fn request_next_package_or_advance(&mut self, game_io: &GameIO) {
        let Some((category, id)) = self.pending_packages.pop() else {
            if self.started_connection {
                self.send(game_io, SyncDataPacket::RequestSave);
                self.build_identity_list();
            } else {
                // flip around and send our package list
                // this brings control back to the device that started the connection
                self.send_package_list(game_io);
            }

            return;
        };

        self.send(game_io, SyncDataPacket::RequestPackage { category, id });

        let remaining = self
            .total_packages
            .saturating_sub(self.pending_packages.len());

        let globals = game_io.resource::<Globals>().unwrap();
        let message = globals.translate_with_args(
            "sync-data-downloading-package-message",
            vec![
                ("current", remaining.into()),
                ("total", self.total_packages.into()),
            ],
        );

        let message = String::from("\x04") + message.as_str();
        let key = self.textbox.push_doorstop_with_message(message);
        self.doorstop_key = Some(key);
    }

    fn package_zip(game_io: &GameIO, category: PackageCategory, id: &PackageId) -> Vec<u8> {
        let globals = game_io.resource::<Globals>().unwrap();
        let Some(package_info) = globals.package_info(category, PackageNamespace::Local, id) else {
            return vec![];
        };

        if package_info.local_only() {
            packets::zip::compress(&package_info.base_path)
                .inspect_err(|err| {
                    log::error!("Failed to zip {:?} {err:?}", package_info.base_path)
                })
                .unwrap_or_default()
        } else {
            let mod_cache_folder = ResourcePaths::mod_cache_folder();
            let path = format!("{}{}.zip", mod_cache_folder, package_info.hash);
            globals.assets.binary(&path)
        }
    }

    fn build_identity_list(&mut self) {
        let identity_folder = ResourcePaths::identity_folder();

        if let Ok(entries) = std::fs::read_dir(identity_folder) {
            self.pending_identity = entries
                .into_iter()
                .flatten()
                .flat_map(|entry| {
                    Some((
                        // name
                        entry.file_name().to_str().map(|s| s.to_string())?,
                        // time
                        file_creation_time(entry.metadata()).unwrap_or_default(),
                    ))
                })
                .collect();
        }

        // make sure an identity folder exists
        let identity_folder = ResourcePaths::identity_folder();
        let _ = std::fs::create_dir_all(identity_folder);
    }

    fn handle_packets(&mut self, game_io: &mut GameIO) {
        self.handle_sync_packets(game_io);
        self.handle_multicast_packets(game_io);
    }

    fn handle_multicast_packets(&mut self, game_io: &GameIO) {
        if self.sync_comms.is_none() && self.last_broadcast.elapsed() > BROADCAST_RATE {
            let globals = game_io.resource::<Globals>().unwrap();

            globals.network.send_multicast(MulticastPacket::Client {
                name: globals.global_save.nickname.clone(),
                uuid: MulticastPacket::local_uuid(),
            });

            self.last_broadcast = Instant::now();
        }

        while let Ok((addr, packet)) = self.multicast_receiver.try_recv() {
            match packet {
                MulticastPacket::Client { name, uuid } => {
                    // update list if this is new data
                    if self.discovered.get(&addr).is_none_or(|(_, n)| *n != name) {
                        self.discovered.insert(addr, (uuid, name));
                        self.list.set_children(Self::generate_list_children(
                            game_io,
                            &self.event_sender,
                            &self.discovered,
                        ));
                    }
                }
                MulticastPacket::RequestSync { uuid } => {
                    if uuid != MulticastPacket::local_uuid() {
                        // not for us
                        continue;
                    }

                    if self.textbox.is_open() {
                        if self
                            .requesting_connection_with
                            .is_some_and(|(a, _)| a == addr)
                        {
                            // we must've hit sync at the same time
                            // drop connection since this is difficult to handle
                            self.sync_comms = None;
                            self.requesting_connection_with = None;

                            self.textbox.advance_interface(game_io);

                            let globals = game_io.resource::<Globals>().unwrap();
                            let message = globals.translate("sync-data-spider-man-pointing-error");
                            let interface = TextboxMessage::new(message);
                            self.textbox.push_interface(interface);
                            self.doorstop_key.take();
                        }

                        // ignore since we're busy
                        continue;
                    }

                    // start connection to keep it alive until we accept / reject
                    self.accepted_sync = false;
                    self.start_connection(game_io, addr);

                    // ask for permission to sync
                    let sender = self.event_sender.clone();

                    let globals = game_io.resource::<Globals>().unwrap();
                    let question = globals.translate_with_args(
                        "sync-data-accept-sync-question",
                        vec![("name", addr.to_string().into())],
                    );

                    let interface = TextboxQuestion::new(game_io, question, move |accept| {
                        let event = if accept {
                            Event::AcceptSyncWith
                        } else {
                            Event::RejectSyncWith
                        };

                        let _ = sender.send(event);
                    });

                    self.textbox.push_interface(interface);
                    self.textbox.open();

                    // add a doorstop
                    let key = self.textbox.push_doorstop();
                    self.doorstop_key = Some(key);
                }
                _ => {}
            };
        }
    }

    fn handle_sync_packets(&mut self, game_io: &mut GameIO) {
        let Some((send, receiver)) = &self.sync_comms else {
            return;
        };

        if self.accepted_sync && receiver.is_disconnected() {
            self.sync_comms = None;

            let globals = game_io.resource::<Globals>().unwrap();
            let message = globals.translate("sync-data-disconnect-error");
            let interface = TextboxMessage::new(message);
            self.textbox.push_interface(interface);
            self.doorstop_key.take();
            return;
        }

        // recycle multicast broadcast for sync data heartbeat
        // multicast broadcast is disabled when sync comms are open
        if self.last_broadcast.elapsed() > BROADCAST_RATE {
            if let Some((_, uuid)) = self.requesting_connection_with {
                // request sync over multicast
                let globals = game_io.resource::<Globals>().unwrap();
                let network = &globals.network;
                network.send_multicast(MulticastPacket::RequestSync { uuid });
            }

            send(Reliability::Reliable, SyncDataPacket::Heartbeat);
            self.last_broadcast = Instant::now();
        }

        if !self.accepted_sync {
            return;
        }

        let receiver = receiver.clone();

        while let Ok(packet) = receiver.try_recv() {
            match packet {
                SyncDataPacket::Heartbeat => {
                    // this is only used to keep the connection alive
                }
                SyncDataPacket::RejectSync => {
                    self.sync_comms = None;
                    self.requesting_connection_with = None;
                    self.doorstop_key.take();

                    while self.textbox.remaining_interfaces() > 0 {
                        self.textbox.advance_interface(game_io);
                    }

                    let globals = game_io.resource::<Globals>().unwrap();
                    let message = globals.translate("sync-data-rejected-message");
                    let interface = TextboxMessage::new(message);
                    self.textbox.push_interface(interface);
                    self.textbox.open();

                    return;
                }
                SyncDataPacket::AcceptSync => {
                    self.requesting_connection_with = None;
                    self.started_connection = true;

                    self.send_package_list(game_io);
                }
                SyncDataPacket::PackageList { list } => {
                    log::debug!("Received package list");

                    let globals = game_io.resource::<Globals>().unwrap();
                    self.pending_packages = list
                        .into_iter()
                        .filter(|(category, id)| {
                            // only download packages we don't have
                            // ignore hashes / dates to avoid overwriting a mod someone is working on
                            let namespace = PackageNamespace::Local;
                            globals.package_info(*category, namespace, id).is_none()
                        })
                        .collect();

                    self.total_packages = self.pending_packages.len();

                    self.request_next_package_or_advance(game_io);
                }
                SyncDataPacket::RequestPackage { category, id } => {
                    log::debug!("Sending package {category:?} {id:?}");

                    let zip_bytes = Self::package_zip(game_io, category, &id);

                    self.send(
                        game_io,
                        SyncDataPacket::Package {
                            category,
                            id,
                            zip_bytes,
                        },
                    );
                }
                SyncDataPacket::Package {
                    category,
                    id,
                    zip_bytes,
                } => {
                    log::debug!("Received package {id:?}");

                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    let path = globals.resolve_package_download_path(category, &id);

                    packets::zip::extract_to(&zip_bytes, &path);
                    globals.unload_package(category, PackageNamespace::Local, &id);
                    globals.load_package(category, PackageNamespace::Local, &path);

                    self.request_next_package_or_advance(game_io);
                }
                SyncDataPacket::RequestSave => {
                    log::debug!("Sending save data");

                    let globals = game_io.resource::<Globals>().unwrap();

                    let message = globals.translate("sync-data-syncing-save-message");
                    let key = self.textbox.push_doorstop_with_message(message);
                    self.doorstop_key = Some(key);

                    let save = serialize(&globals.global_save);
                    self.send(game_io, SyncDataPacket::Save { save });

                    // build identity list on the remote device
                    self.build_identity_list();
                }
                SyncDataPacket::Save { save } => {
                    log::debug!("Received save data");

                    let globals = game_io.resource_mut::<Globals>().unwrap();

                    if let Ok(remote_save) = deserialize::<GlobalSave>(&save) {
                        if self.started_connection {
                            globals.global_save.sync(remote_save);
                            globals.global_save.save();

                            log::debug!("Sending save data");

                            let save = serialize(&globals.global_save);
                            self.send(game_io, SyncDataPacket::Save { save })
                        } else {
                            // the other client handled sync, use their save
                            globals.global_save = remote_save;
                            globals.global_save.save();
                        }
                    } else {
                        let message = globals.translate("sync-data-read-remote-save-error");

                        log::debug!("{message}");

                        let interface = TextboxMessage::new(message);
                        self.textbox.push_interface(interface);
                        self.textbox.open();
                    }

                    if self.started_connection {
                        // only one side needs to message here
                        // we share saves at the same time, and the first device will kick this off
                        self.send(game_io, SyncDataPacket::RequestIdentity);
                    }
                }
                SyncDataPacket::RequestIdentity => {
                    let next_identity = self.pending_identity.pop();
                    let packet = next_identity
                        .and_then(|(file_name, created_time)| {
                            log::debug!("Sending identity for {}", uri_decode(&file_name)?);

                            let path = ResourcePaths::identity_folder() + &file_name;
                            let data = std::fs::read(&path)
                                .inspect_err(|err| log::error!("Failed to read {path}: {err:?}"))
                                .ok()?;

                            Some(SyncDataPacket::Identity {
                                file_name,
                                data,
                                created_time,
                            })
                        })
                        .unwrap_or(if self.started_connection {
                            SyncDataPacket::Complete { cancelled: false }
                        } else {
                            // start requesting identity back
                            SyncDataPacket::RequestIdentity
                        });

                    self.send(game_io, packet);
                }
                SyncDataPacket::Identity {
                    file_name,
                    data,
                    created_time,
                } => {
                    if let Some(decoded_name) = uri_decode(&file_name) {
                        log::debug!("Received identity {decoded_name}");

                        // re-encode for safety
                        let path = ResourcePaths::identity_folder() + &uri_encode(&decoded_name);

                        // use the older identity file
                        let local_created_time = file_creation_time(std::fs::metadata(&path));

                        if local_created_time.is_none_or(|time| created_time < time)
                            && let Err(err) = std::fs::write(path, data)
                        {
                            log::error!("Failed to save identity for {decoded_name}: {err:?}");
                        }

                        // request more identity
                        self.send(game_io, SyncDataPacket::RequestIdentity);
                    }
                }
                SyncDataPacket::Complete { cancelled } => {
                    self.sync_comms = None;
                    self.doorstop_key.take();

                    // update avatar to match the latest save
                    self.textbox.use_navigation_avatar(game_io);

                    let message_key = if cancelled {
                        "sync-data-cancelled-message"
                    } else {
                        "sync-data-complete-message"
                    };

                    let globals = game_io.resource::<Globals>().unwrap();

                    let message = globals.translate(message_key);
                    let interface = TextboxMessage::new(message);
                    self.textbox.push_interface(interface);
                    self.textbox.open();
                    return;
                }
            }
        }
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        self.ui_input_tracker.update(game_io);
        self.textbox.update(game_io);

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        let list_focusable = !self.discovered.is_empty() && !self.textbox.is_open();
        self.list.set_focused(list_focusable);

        if self.textbox.is_open() || game_io.is_in_transition() {
            return;
        }

        self.list.update(game_io, &self.ui_input_tracker);

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let transition = crate::transitions::new_sub_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
        }
    }

    fn handle_events(&mut self, game_io: &GameIO) {
        if self.next_scene.is_some() || game_io.is_in_transition() {
            return;
        }

        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::RequestSyncWith { address, uuid } => {
                    self.requesting_connection_with = Some((address, uuid));
                    self.accepted_sync = true;
                    self.start_connection(game_io, address);

                    // notify user
                    let globals = game_io.resource::<Globals>().unwrap();

                    let message = globals.translate("sync-data-requesting-message");
                    let key = self.textbox.push_doorstop_with_message(message.to_string());
                    self.doorstop_key = Some(key);
                    self.textbox.open();
                }
                Event::AcceptSyncWith => {
                    self.accepted_sync = true;
                    self.started_connection = false;
                    self.send(game_io, SyncDataPacket::AcceptSync);

                    // notify user
                    let globals = game_io.resource::<Globals>().unwrap();

                    let message = globals.translate("sync-data-syncing-message");
                    let key = self.textbox.push_doorstop_with_message(message.to_string());
                    self.doorstop_key = Some(key);
                    self.textbox.open();
                }
                Event::RejectSyncWith => {
                    self.send(game_io, SyncDataPacket::RejectSync);
                    self.doorstop_key.take();
                    self.sync_comms = None;
                }
                Event::SyncConnectionResolved(comms) => {
                    self.sync_comms = comms;
                }
            }
        }
    }
}

impl Scene for SyncDataScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.handle_packets(game_io);
        self.handle_input(game_io);
        self.handle_events(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        self.frame.draw(&mut sprite_queue);
        self.scene_title.draw(game_io, &mut sprite_queue);

        // draw list
        self.list.draw(game_io, &mut sprite_queue);

        // draw local address
        let globals = game_io.resource::<Globals>().unwrap();
        let network_details = globals.network.details();
        let address = network_details.local_address().unwrap_or_default();

        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.bounds.set_position(self.local_address_position);

        let metrics = text_style.measure(address);
        text_style.bounds.x -= metrics.size.x;
        text_style.draw(game_io, &mut sprite_queue, address);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn file_creation_time(metadata_result: std::io::Result<std::fs::Metadata>) -> Option<u64> {
    let metadata = metadata_result.ok()?;
    let creation_instant = metadata.created().ok()?;
    Some(creation_instant.elapsed().ok()?.as_secs())
}
