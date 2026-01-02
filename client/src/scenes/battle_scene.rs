use crate::battle::*;
use crate::bindable::SpriteColorMode;
use crate::packages::PackageNamespace;
use crate::render::ui::{FontName, TextStyle, Textbox, TextboxMessage, TextboxQuestion};
use crate::render::*;
use crate::resources::*;
use crate::saves::{
    BattleRecording, PlayerInputBuffer, RecordedPreview, RecordedRollback, RecordedSimulationFlow,
};
use framework::prelude::*;
use packets::structures::PackageId;
use packets::{
    ClientPacket, NetplayBufferItem, NetplayPacket, NetplayPacketData, NetplaySignal, Reliability,
};
use std::collections::VecDeque;
use std::sync::Arc;

const SLOW_COOLDOWN: FrameTime = INPUT_BUFFER_LIMIT as FrameTime;
const DEFAULT_LEAD_TOLERANCE: usize = 2;
const PING_RATE: FrameTime = 60;

pub enum BattleEvent {
    Description(Arc<str>),
    DescribeCard(PackageNamespace, PackageId),
    RequestFlee,
    AttemptFlee,
    FleeResult(bool),
    CompleteFlee(bool),
}

struct PlayerController {
    connected: bool,
    ping_sent: Option<Instant>,
    rtt: f32,
    buffer: PlayerInputBuffer,
    lead_tolerance: usize,
    average_frame_time: f32,
}

struct Backup {
    simulation: BattleSimulation,
    state: Box<dyn State>,
}

pub struct BattleScene {
    meta: BattleMeta,
    comms: BattleComms,
    server_messages: usize,
    statistics_callback: Option<BattleStatisticsCallback>,
    recording: Option<BattleRecording>,
    recording_preview: Option<RecordedPreview>,
    ui_camera: Camera,
    textbox: Textbox,
    textbox_is_blocking_input: bool,
    pending_signals: Vec<NetplaySignal>,
    synced_time: FrameTime,
    resources: SharedBattleResources,
    simulation: BattleSimulation,
    state: Box<dyn State>,
    backups: VecDeque<Backup>,
    player_controllers: Vec<PlayerController>,
    connected_count: usize,
    local_index: Option<usize>,
    slow_cooldown: FrameTime,
    frame_by_frame_debug: bool,
    resimulating: bool,
    draw_player_indices: bool,
    already_snapped: bool,
    is_playing_back_recording: bool,
    playback_multiplier: u8,
    playback_flow: Option<RecordedSimulationFlow>,
    exiting: bool,
    next_scene: NextScene,
}

impl BattleScene {
    pub fn new(game_io: &mut GameIO, mut props: BattleProps) -> Self {
        let mut is_playing_back_recording = false;

        let mut initial_external_events = Default::default();
        #[allow(unused_mut)]
        let mut playback_flow = None;

        if let Some(mut recording) = props.meta.try_load_recording(game_io) {
            initial_external_events = std::mem::take(&mut recording.external_events);

            #[cfg(feature = "record_simulation_flow")]
            if let Some(mut recorded_flow) = recording.simulation_flow.take() {
                recorded_flow.current_step = 0;
                playback_flow = Some(recorded_flow);
            }

            props.meta = BattleMeta::from_recording(game_io, recording);
            is_playing_back_recording = true;
        } else {
            // remove recording namespaces to prevent interference with other namespaces
            // recording namespaces have precedence over other namespaces
            let globals = game_io.resource_mut::<Globals>().unwrap();
            globals.remove_namespace(PackageNamespace::RecordingServer);

            // prevent unused netplay packages from overriding local packages
            if let Some(local_setup) = props.meta.player_setups.iter().find(|s| s.local) {
                globals.remove_namespace(local_setup.namespace());
            }

            globals.assets.remove_unused_virtual_zips();
        }

        if props.simulation_and_resources.is_none() {
            // init simulation
            props.load_encounter(game_io);
        }

        // take simulation and resources
        let (mut simulation, mut resources) = props.simulation_and_resources.unwrap();

        // load initial external events
        resources.external_events.load(initial_external_events);

        let mut meta = props.meta;
        let comms = props.comms;

        // sort player setups for consistent execution order on every client
        meta.player_setups.sort_by_key(|setup| setup.index);
        resources.load_player_vms(game_io, &mut simulation, &meta);

        // init recording struct
        let recording = if meta.recording_enabled {
            Some(BattleRecording::new(&meta))
        } else {
            None
        };

        // load the players in the correct order
        let player_setups = &meta.player_setups;
        let mut player_controllers = Vec::with_capacity(player_setups.len());
        let local_index = if is_playing_back_recording {
            None
        } else {
            player_setups
                .iter()
                .find(|setup| setup.local)
                .map(|setup| setup.index)
        };

        let config = resources.config.borrow();
        let target_frame_time = game_io.target_duration().as_secs_f32();

        for setup in player_setups {
            simulation.inputs[setup.index].set_input_delay(setup.buffer.delay());

            player_controllers.push(PlayerController {
                connected: !is_playing_back_recording && setup.connected,
                ping_sent: None,
                rtt: 0.0,
                buffer: setup.buffer.clone(),
                lead_tolerance: DEFAULT_LEAD_TOLERANCE,
                average_frame_time: target_frame_time,
            });

            if !setup.memories.is_empty() {
                let memories = EntityMemories::new(setup.memories.clone());
                simulation.memories.insert(setup.index, memories);
            }

            if !config.spectators.contains(&setup.index) {
                let result = Player::load(game_io, &resources, &mut simulation, setup);

                if let Err(e) = result {
                    log::error!("{e}");
                }
            }
        }

        let connected_count = player_controllers
            .iter()
            .enumerate()
            .filter(|(i, controller)| controller.connected && Some(*i) != local_index)
            .count();

        std::mem::drop(config);

        Player::initialize_uninitialized(&resources, &mut simulation);

        Self {
            meta,
            comms,
            server_messages: 0,
            statistics_callback: props.statistics_callback,
            recording,
            recording_preview: None,
            ui_camera: Camera::new_ui(game_io),
            textbox: Textbox::new_overworld(game_io)
                .with_transition_animation_enabled(!is_playing_back_recording),
            textbox_is_blocking_input: false,
            pending_signals: Vec::new(),
            synced_time: 0,
            resources,
            simulation,
            state: Box::new(IntroState::new()),
            backups: VecDeque::new(),
            player_controllers,
            connected_count,
            local_index,
            slow_cooldown: 0,
            frame_by_frame_debug: false,
            resimulating: false,
            draw_player_indices: false,
            already_snapped: false,
            is_playing_back_recording,
            playback_multiplier: 1,
            playback_flow,
            exiting: false,
            next_scene: NextScene::None,
        }
    }

    fn is_offline(&self) -> bool {
        self.player_controllers.len() == 1 && self.comms.server.is_none()
    }

    fn update_textbox(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.resources.event_receiver.try_recv() {
            match event {
                BattleEvent::Description(description) => {
                    let interface = TextboxMessage::new(description.to_string());
                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                BattleEvent::DescribeCard(ns, package_id) => {
                    let globals = game_io.resource::<Globals>().unwrap();

                    let Some(package) = globals.card_packages.package_or_fallback(ns, &package_id)
                    else {
                        continue;
                    };

                    let description = if package.long_description.is_empty() {
                        package.description.to_string()
                    } else {
                        package.long_description.to_string()
                    };

                    self.textbox.use_blank_avatar(game_io);
                    let interface = TextboxMessage::new(description);
                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                BattleEvent::RequestFlee => {
                    let is_boss_battle = self.simulation.statistics.boss_battle;
                    let can_run = !is_boss_battle;

                    self.textbox.use_navigation_avatar(game_io);

                    if can_run {
                        // confirm
                        let event_sender = self.resources.event_sender.clone();
                        let globals = game_io.resource::<Globals>().unwrap();
                        let text = globals.translate("battle-run-question");

                        let interface = TextboxQuestion::new(game_io, text, move |yes| {
                            if yes {
                                event_sender.send(BattleEvent::AttemptFlee).unwrap();
                            }
                        });

                        self.textbox.push_interface(interface);
                    } else {
                        // can't run
                        let globals = game_io.resource::<Globals>().unwrap();
                        let text = globals.translate("battle-run-disabled-message");
                        let interface = TextboxMessage::new(text);

                        self.textbox.push_interface(interface);
                    }

                    self.textbox.open();
                }
                BattleEvent::AttemptFlee => {
                    // pass signal for BattleSimulation::handle_local_signals to resolve the success
                    self.pending_signals.push(NetplaySignal::AttemptingFlee);
                }
                BattleEvent::FleeResult(success) => {
                    let event_sender = self.resources.event_sender.clone();
                    let message_key = if success {
                        "battle-run-success"
                    } else {
                        "battle-run-failed"
                    };

                    let globals = game_io.resource::<Globals>().unwrap();
                    let message =
                        String::from("\x01...\x01") + globals.translate(message_key).as_str();

                    let interface = TextboxMessage::new(message).with_callback(move || {
                        event_sender
                            .send(BattleEvent::CompleteFlee(success))
                            .unwrap();
                    });

                    self.textbox.use_navigation_avatar(game_io);
                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                BattleEvent::CompleteFlee(success) => {
                    if success {
                        self.exit(game_io, true);
                    }

                    self.pending_signals.push(NetplaySignal::CompletedFlee);
                }
            }
        }

        if self.textbox.is_complete() {
            self.textbox.close()
        }

        if self.textbox.is_open() {
            self.textbox_is_blocking_input = true;
        } else if self.textbox_is_blocking_input {
            // we only stop blocking when the player lifts up every button
            let input_util = InputUtil::new(game_io);

            self.textbox_is_blocking_input =
                Input::BATTLE.iter().any(|&input| input_util.is_down(input));
        }

        self.textbox.update(game_io);
    }

    fn handle_server_messages(&mut self) {
        let Some((_, receiver)) = self.comms.server.as_ref() else {
            return;
        };

        while let Ok((battle_id, message)) = receiver.try_recv() {
            if battle_id != self.comms.remote_id {
                // ignore messages meant for other battles
                continue;
            }

            let server_events = &mut self.resources.external_events.server;

            server_events.update_event(self.server_messages, message);

            let ack_signal = NetplaySignal::AcknowledgeServerMessage(self.server_messages);
            self.pending_signals.push(ack_signal);

            self.server_messages += 1;
        }
    }

    fn handle_packets(&mut self, game_io: &mut GameIO) {
        let mut packets = Vec::new();
        let mut pending_removal = Vec::new();

        'main_loop: for (i, (index, receiver)) in self.comms.receivers.iter().enumerate() {
            while let Ok(packet) = receiver.try_recv() {
                if index.is_some() && Some(packet.index) != *index {
                    // ignore obvious impersonation cheat
                    continue;
                }

                let controller = self.player_controllers.get(packet.index);

                if controller.is_none_or(|c| !c.connected) {
                    // ignore packets from players that have already disconnected
                    continue;
                }

                let is_disconnect = matches!(
                    &packet.data,
                    NetplayPacketData::Buffer { data, .. } if data.signals.contains(&NetplaySignal::Disconnect)
                );

                packets.push(packet);

                if is_disconnect {
                    self.connected_count -= 1;

                    if self.connected_count == 0 {
                        // break to prevent receiving extra packets from the fallback receiver
                        // these extra packets are likely for future scenes
                        break 'main_loop;
                    }
                }
            }

            if receiver.is_disconnected() {
                pending_removal.push(i);
            }
        }

        // remove disconnected receivers
        let mut possible_disconnect_desync = false;

        for i in pending_removal.into_iter().rev() {
            let (player_index, _) = self.comms.receivers.remove(i);

            // disconnect an individual
            // unless this is a fallback connection, then disconnect all players
            let range = player_index
                .map(|index| index..index + 1)
                .unwrap_or(0..self.player_controllers.len());

            for index in range {
                let Some(controller) = self.player_controllers.get_mut(index) else {
                    break;
                };

                if controller.connected {
                    packets.push(NetplayPacket::new_disconnect_signal(index));
                    possible_disconnect_desync = true;
                }
            }
        }

        if possible_disconnect_desync {
            // possible desync when there's another player we need to sync a disconnect with
            log::error!("Possible desync from a player disconnect without a Disconnect signal");
        }

        if self.connected_count == 0 {
            // no need to store these, helps prevent reading too many packets from the fallback receiver
            self.comms.receivers.clear();
        }

        let frame_start_instant = game_io.frame_start_instant();
        let target_frame_time = game_io.target_duration().as_secs_f32();
        let mut resimulation_time = self.simulation.time;

        for packet in packets {
            if let Some(resim_time) =
                self.handle_packet(frame_start_instant, target_frame_time, packet)
            {
                resimulation_time = resimulation_time.min(resim_time);
            }
        }

        self.resimulate(game_io, resimulation_time);

        // after resolving packets we should see if we're too far ahead of other players
        // and decide whether we should slow down
        self.resolve_slowdown(game_io);
    }

    fn handle_packet(
        &mut self,
        frame_start_instant: Instant,
        target_frame_time: f32,
        packet: NetplayPacket,
    ) -> Option<FrameTime> {
        let index = packet.index;
        let mut resimulation_time = None;

        match packet.data {
            NetplayPacketData::Buffer { data, frame_time } => {
                if let Some(controller) = self.player_controllers.get_mut(index) {
                    // check disconnect
                    if data.signals.contains(&NetplaySignal::Disconnect) {
                        controller.connected = false;
                        self.resources.external_events.dropped_player(index);
                    }

                    if let Some(input) = self.simulation.inputs.get(index)
                        && !input.matches(&data)
                    {
                        // resolve the time of the input if it differs from our simulation
                        resimulation_time =
                            Some(self.synced_time + controller.buffer.len() as FrameTime);
                    }

                    if !data.signals.is_empty() {
                        log::debug!("Received {:?} from {index}", data.signals);
                    }

                    controller.buffer.push_last(data);

                    // mimicking packet_sender.rs
                    // we cap frame_time at target_frame_time since we care more about dips than headroom
                    controller.average_frame_time = smooth_average_f32(
                        controller.average_frame_time,
                        frame_time.max(target_frame_time),
                    );
                }
            }
            NetplayPacketData::Ping => {
                self.send(index, NetplayPacketData::Pong { sender: index });
            }
            NetplayPacketData::Pong { sender } => {
                if self.local_index == Some(sender)
                    && let Some(controller) = self.player_controllers.get_mut(index)
                    && let Some(ping_sent_time) = controller.ping_sent.take()
                {
                    let new_rtt = (frame_start_instant - ping_sent_time).as_secs_f32();

                    if controller.rtt == 0.0 {
                        controller.rtt = new_rtt;
                    } else {
                        controller.rtt = smooth_average_f32(controller.rtt, new_rtt);
                    }

                    let frame_rtt = (controller.rtt / target_frame_time).ceil() as usize;
                    // our lead tolerance is half rtt + 1
                    // as we expect the time to send to us to be close to half the round trip
                    controller.lead_tolerance = frame_rtt.div_ceil(2) + 1;
                }
            }
            NetplayPacketData::Heartbeat => {}
            data => {
                let name: &'static str = (&data).into();

                log::error!(
                    "Unexpected packet received during battle, received: {name} from {index}"
                );
            }
        }

        resimulation_time
    }

    fn resolve_slowdown(&mut self, game_io: &GameIO) {
        if self.slow_cooldown > 0 {
            self.slow_cooldown -= 1;
        }

        let target_buffer_len = self
            .local_index
            .map(|index| self.player_controllers[index].buffer.len())
            .unwrap_or_else(|| {
                let globals = game_io.resource::<Globals>().unwrap();
                globals.config.input_delay as usize
            });

        let mut should_slow = false;

        for (i, controller) in self.player_controllers.iter().enumerate() {
            if !controller.connected || self.local_index == Some(i) {
                continue;
            }

            #[cfg(debug_assertions)]
            println!(
                "controller: {i}, buffer: {}, tolerance: {}, b+t: {}, b+t target: {}, rtt: {:.0}ms, fps: {:.1}",
                controller.buffer.len(),
                controller.lead_tolerance,
                controller.buffer.len() + controller.lead_tolerance,
                target_buffer_len,
                controller.rtt * 1000.0,
                1.0 / controller.average_frame_time
            );

            // try to maintain a buffer len to stay in the past
            if controller.buffer.len() + controller.lead_tolerance < target_buffer_len {
                should_slow = true;
            }
        }

        if should_slow && self.slow_cooldown == 0 {
            #[cfg(debug_assertions)]
            println!("- slowing down");
            self.slow_cooldown = SLOW_COOLDOWN;
        }
    }

    fn send(&self, mut to_index: usize, data: NetplayPacketData) {
        let Some(index) = self.local_index else {
            return;
        };

        if index == to_index {
            log::warn!("Attempted to send netplay packet to self");
            return;
        }

        if to_index > index {
            to_index -= 1;
        }

        let senders = &self.comms.senders;
        let Some(send) = senders.get(to_index).or(senders.last()) else {
            return;
        };

        let packet = NetplayPacket { index, data };
        send(packet);
    }

    fn broadcast(&self, data: NetplayPacketData) {
        if let Some(index) = self.local_index {
            for send in &self.comms.senders {
                let packet = NetplayPacket {
                    index,
                    data: data.clone(),
                };

                send(packet);
            }
        }
    }

    fn input_synced(&self) -> bool {
        #[allow(clippy::unused_enumerate_index)]
        self.player_controllers
            .iter()
            .enumerate()
            .all(|(_i, controller)| {
                #[cfg(feature = "record_simulation_flow")]
                if let Some(playback_flow) = &self.playback_flow {
                    let player_count = self.player_controllers.len();
                    let limit = playback_flow.get_buffer_limit(player_count, _i);

                    if limit.is_some_and(|limit| limit == 0) {
                        return false;
                    }
                }

                !controller.connected || !controller.buffer.is_empty()
            })
    }

    fn handle_local_input(&mut self, game_io: &GameIO) {
        let Some(local_index) = self.local_index else {
            return;
        };

        let input_util = InputUtil::new(game_io);

        let Some(local_controller) = self.player_controllers.get_mut(local_index) else {
            return;
        };

        // gather input
        let mut pressed = Vec::new();

        if !self.textbox_is_blocking_input && !game_io.input().is_key_down(Key::F3) {
            for input in Input::BATTLE {
                if input_util.is_down(input) {
                    pressed.push(input);
                }
            }
        }

        // create buffer item
        let data = NetplayBufferItem {
            pressed,
            signals: std::mem::take(&mut self.pending_signals),
        };

        // update local buffer
        local_controller.buffer.push_last(data.clone());

        self.broadcast(NetplayPacketData::Buffer {
            data,
            frame_time: game_io.frame_duration().as_secs_f32(),
        });

        self.try_ping();
    }

    fn try_ping(&mut self) {
        if self.simulation.time % PING_RATE != 0 {
            return;
        }

        let pong_pending = self
            .player_controllers
            .iter()
            .enumerate()
            .any(|(i, controller)| {
                self.local_index != Some(i)
                    && controller.connected
                    && controller.ping_sent.is_some()
            });

        if pong_pending {
            return;
        }

        self.broadcast(NetplayPacketData::Ping);

        let now = Instant::now();
        for controller in &mut self.player_controllers {
            controller.ping_sent = Some(now);
        }
    }

    fn record_buffer_limits(&mut self) {
        let Some(recording) = &mut self.recording else {
            return;
        };

        let Some(simulation_flow) = &mut recording.simulation_flow else {
            return;
        };

        for controller in &self.player_controllers {
            let len = if controller.connected {
                controller.buffer.len()
            } else {
                INPUT_BUFFER_LIMIT
            };

            simulation_flow.buffer_limits.push(len as _);
        }

        simulation_flow.current_step += 1;
    }

    fn load_input(&mut self) {
        let input_index = (self.simulation.time - self.synced_time) as usize;

        for (index, player_input) in self.simulation.inputs.iter_mut().enumerate() {
            player_input.flush();

            if let Some(playback_flow) = &self.playback_flow {
                let player_count = self.player_controllers.len();
                let limit = playback_flow.get_buffer_limit(player_count, index);

                if limit.is_some_and(|limit| input_index >= limit) {
                    // avoid loading input past the allowed limit
                    continue;
                }
            }

            if let Some(controller) = self.player_controllers.get(index)
                && let Some(data) = controller.buffer.get(input_index)
            {
                player_input.load_data(data.clone());
            }
        }

        if !self.resimulating && self.input_synced() {
            // prevent buffers from infinitely growing
            for (i, controller) in self.player_controllers.iter_mut().enumerate() {
                let Some(buffer_item) = controller.buffer.pop_next() else {
                    continue;
                };

                for signal in &buffer_item.signals {
                    if let NetplaySignal::AcknowledgeServerMessage(id) = signal {
                        let server_events = &mut self.resources.external_events.server;
                        server_events.acknowledge_event(*id, i);
                    }
                }

                // record input
                if let Some(recording) = &mut self.recording {
                    recording.player_setups[i].buffer.push_last(buffer_item);
                }
            }

            let external_events = &mut self.resources.external_events;

            // record external events
            if let Some(recording) = &mut self.recording {
                let new_event_iter = external_events
                    .events_for_time(self.synced_time)
                    .map(|event| (self.synced_time, event.clone()));

                recording.external_events.extend(new_event_iter);
            }

            // progress external events, and time
            external_events.tick(self.synced_time, self.connected_count + 1);
            self.synced_time += 1;
        }
    }

    fn resimulate(&mut self, game_io: &mut GameIO, start_time: FrameTime) {
        let local_time = self.simulation.time;

        if start_time >= local_time {
            // no need to resimulate, we're behind
            return;
        }

        if let Some(recording) = &mut self.recording
            && let Some(recorded_flow) = &mut recording.simulation_flow
        {
            recorded_flow.rollbacks.push_back(RecordedRollback {
                flow_step: recorded_flow.current_step,
                resimulate_time: start_time,
            });

            self.record_buffer_limits();
        }

        // rollback
        let steps = (local_time - start_time) as usize;
        self.rollback(game_io, steps);

        self.simulation.is_resimulation = true;
        self.resimulating = true;

        // resimulate until we're caught up to our previous time
        while self.simulation.time < local_time {
            // avoiding snapshots for the first frame as it's still retained
            self.simulate(game_io);
        }

        self.simulation.is_resimulation = false;
        self.resimulating = false;
    }

    fn rewind(&mut self, game_io: &mut GameIO, mut steps: usize) {
        // taking an extra step back as we'll resimulate one step forward again
        // this ensures the snapshot is popped as repeated self.rollback(1) has no effect
        if !self.already_snapped {
            steps += 1;
        }

        if self.backups.len() < steps {
            return;
        }

        self.resimulating = true;
        self.rollback(game_io, steps);
        self.simulate(game_io);
        self.resimulating = false;
        self.synced_time = self.simulation.time;

        // undo input committed to the recording
        if let Some(recording) = &mut self.recording {
            // give up on recording rollbacks
            recording.simulation_flow = None;

            for setup in &mut recording.player_setups {
                for _ in 0..steps - 1 {
                    setup.buffer.delete_last();
                }

                debug_assert_eq!(self.synced_time as usize, setup.buffer.len());
            }

            if let Some(index) = self.local_index {
                let buffer = &self.player_controllers[index].buffer;
                debug_assert_eq!(buffer.len(), buffer.delay());
            }
        }
    }

    fn rollback(&mut self, game_io: &GameIO, steps: usize) {
        self.resources.vm_manager.rollback(steps);

        for _ in 0..steps - 1 {
            self.backups.pop_back();
        }

        let backup = self.backups.back_mut().unwrap();
        self.simulation = backup.simulation.clone(game_io);
        self.state = backup.state.clone_box();
        self.already_snapped = true;
    }

    fn simulate(&mut self, game_io: &mut GameIO) {
        if !self.already_snapped {
            self.resources.vm_manager.snap();

            let mut simulation_clone = self.simulation.clone(game_io);

            // use the clone as self.simulation
            // gives us a fresh Archetype order for hecs::World
            // should keep all clients in sync for Archetypes
            std::mem::swap(&mut simulation_clone, &mut self.simulation);

            self.backups.push_back(Backup {
                simulation: simulation_clone,
                state: self.state.clone_box(),
            });
        }

        self.already_snapped = false;

        self.load_input();

        // update simulation
        if let Some(state) = self.state.next_state(game_io) {
            self.state = state;
        }

        let simulation = &mut self.simulation;
        let state = &mut *self.state;
        let resources = &mut self.resources;

        if let Some(index) = self.local_index {
            simulation.handle_local_signals(index, resources);
        }

        simulation.pre_update(game_io, resources, state);

        // must run before state, as the intro state will hide entities
        if self.recording.is_some() && self.recording_preview.is_none() {
            self.recording_preview = Some(simulation.render_preview(game_io));
        }

        state.update(game_io, resources, simulation);
        simulation.post_update(game_io, resources);

        // drop synced / canonicalized backups, additionally send any associated external messages
        if self.backups.len() > INPUT_BUFFER_LIMIT {
            let backup = self.backups.pop_front();
            let server_comms = self.comms.server.as_ref();

            if let (Some(backup), Some((send, _))) = (backup, server_comms) {
                let messages = backup.simulation.external_outbox;

                for message in messages {
                    send(
                        Reliability::ReliableOrdered,
                        ClientPacket::BattleMessage { message },
                    );
                }
            }
        }

        #[cfg(feature = "record_every_frame")]
        self.record_frame(game_io);
    }

    #[cfg(feature = "record_every_frame")]
    fn record_frame(&mut self, game_io: &mut GameIO) {
        let mut capture_pass = CapturePass::new(game_io, Some("Capture"), RESOLUTION_F.as_uvec2());
        let mut render_pass = capture_pass.create_render_pass();

        self.draw(game_io, &mut render_pass);

        render_pass.flush();

        let frame_number = self.simulation.time;
        let image_future = capture_pass.capture(game_io);

        game_io
            .spawn_local_task(async move {
                let Some(image) = image_future.await else {
                    log::warn!("Failed to capture frame {frame_number}");
                    return;
                };

                // maybe we shouldn't use a thread and just block to avoid writing too fast
                std::thread::spawn(move || {
                    let capture_folder = "./_capture";

                    let _ = std::fs::create_dir(capture_folder);
                    let path = format!("./{capture_folder}/{frame_number}.png");

                    let file = match std::fs::File::create(&path) {
                        Ok(file) => file,
                        Err(e) => {
                            log::error!("Failed to save capture to {path:?}: {e}");
                            return;
                        }
                    };

                    let png_encoder = image::codecs::png::PngEncoder::new(file);

                    if let Err(e) = image.write_with_encoder(png_encoder) {
                        log::error!("Failed to save capture to {path:?}: {e}");
                    }
                });
            })
            .detach();
    }

    fn detect_debug_hotkeys(&mut self, game_io: &mut GameIO) {
        if !game_io.input().is_key_down(Key::F3) {
            return;
        }

        // list vms by memory usage
        if game_io.input().was_key_just_pressed(Key::V) {
            self.resources.vm_manager.print_memory_usage();
        }

        if game_io.input().was_key_just_pressed(Key::I) {
            self.draw_player_indices = !self.draw_player_indices;
        }
    }

    fn exit(&mut self, game_io: &GameIO, fleeing: bool) {
        self.exiting = true;

        if !self.is_playing_back_recording {
            self.pending_signals.push(NetplaySignal::Disconnect);
        }

        if let Some(statistics_callback) = self.statistics_callback.take() {
            self.simulation.wrap_up_statistics();
            let mut statistics = self.simulation.statistics.clone();
            statistics.ran = fleeing;
            statistics_callback(Some(statistics));
        }

        // clean up music stack
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.pop_music_stack();
    }

    fn remaining_replay_buffer(&self) -> FrameTime {
        let controller_iter = self.player_controllers.iter();
        controller_iter
            .map(|controller| controller.buffer.len() as FrameTime)
            .max()
            .unwrap_or_default()
    }

    fn core_update(&mut self, game_io: &mut GameIO) {
        // replay rollbacks
        loop {
            let Some(playback_flow) = &mut self.playback_flow else {
                break;
            };

            let Some(recorded_rollback) = playback_flow.rollbacks.front().cloned() else {
                break;
            };

            if playback_flow.current_step != recorded_rollback.flow_step {
                break;
            }

            self.resimulate(game_io, recorded_rollback.resimulate_time - 1);

            let Some(playback_flow) = &mut self.playback_flow else {
                break;
            };

            playback_flow.rollbacks.pop_front();
            playback_flow.current_step += 1;
        }

        let mut input_util = InputUtil::new(game_io);

        let mut can_simulate = if self.is_playing_back_recording {
            // simulate as long as we have input
            self.remaining_replay_buffer() > 0
        } else {
            // simulate as long as we can roll back to the synced time
            self.simulation.time < self.synced_time + INPUT_BUFFER_LIMIT as FrameTime
                || self.input_synced()
        };

        if self.frame_by_frame_debug {
            if input_util.was_just_pressed(Input::RewindFrame) && !self.is_playing_back_recording {
                self.rewind(game_io, 1);
                input_util = InputUtil::new(game_io)
            }

            can_simulate &= input_util.was_just_pressed(Input::AdvanceFrame);

            // exit from frame_by_frame_debug with pause or confirm
            let resume = input_util.was_just_pressed(Input::Pause)
                || input_util.was_just_pressed(Input::Confirm);
            self.frame_by_frame_debug = !resume;
        } else {
            let should_slow_down = self.slow_cooldown == SLOW_COOLDOWN;

            can_simulate &= !should_slow_down;

            self.frame_by_frame_debug = (self.is_playing_back_recording || self.is_offline())
                && (input_util.was_just_pressed(Input::RewindFrame)
                    || input_util.was_just_pressed(Input::AdvanceFrame));
        }

        if can_simulate {
            self.handle_local_input(game_io);
            self.record_buffer_limits();

            self.simulate(game_io);

            if let Some(playback_flow) = &mut self.playback_flow {
                playback_flow.current_step += 1;
            }
        }
    }

    fn handle_exit_requests(&mut self, game_io: &GameIO) {
        let requested_exit = if self.is_playing_back_recording {
            let input_util = InputUtil::new(game_io);
            input_util.was_just_pressed(Input::Cancel)
        } else {
            // use the oldest backup, as we can still rewind and end up not exiting otherwise
            let oldest_backup = self.backups.front();

            oldest_backup
                .map(|backup| backup.simulation.progress)
                .unwrap_or(self.simulation.progress)
                == BattleProgress::Exiting
        };

        if !self.exiting && requested_exit {
            self.exit(game_io, false);
        }

        if self.exiting && !game_io.is_in_transition() && self.pending_signals.is_empty() {
            // need to exit, not currently exiting, no pending signals that must be shared
            // safe to exit
            let transition = crate::transitions::new_battle_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
        }
    }
}

impl Scene for BattleScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();
        globals.audio.push_music_stack();
    }

    fn destroy(&mut self, game_io: &mut GameIO) {
        let globals = game_io.resource_mut::<Globals>().unwrap();

        // save player memory, must occur before the recording eats setups
        if let Some(setup) = self.meta.player_setups.iter().find(|s| s.local) {
            let memories = self
                .simulation
                .memories
                .get(&setup.index)
                .map(|memories| memories.clone_inner());

            globals
                .global_save
                .update_memories(&setup.package_id, memories);
        }

        if let (Some(recording), Some(preview)) =
            (self.recording.take(), self.recording_preview.take())
        {
            let meta = BattleMeta {
                encounter_package_pair: self.meta.encounter_package_pair.clone(),
                data: std::mem::take(&mut self.meta.data),
                seed: self.meta.seed,
                background: self.meta.background.clone(),
                player_setups: std::mem::take(&mut self.meta.player_setups),
                player_count: self.meta.player_count,
                recording_enabled: false,
            };

            globals.battle_recording = Some((meta, recording, preview))
        }
    }

    fn update(&mut self, game_io: &mut GameIO) {
        if !self.is_playing_back_recording {
            self.update_textbox(game_io);
            self.handle_packets(game_io);
            self.handle_server_messages();
        }

        self.core_update(game_io);

        // multiply speed while holding confirm in a replay
        let input_util = InputUtil::new(game_io);
        if self.is_playing_back_recording && input_util.is_down(Input::Confirm) {
            if input_util.was_just_pressed(Input::Left) {
                self.playback_multiplier -= 1;
            }

            if input_util.was_just_pressed(Input::Right) {
                self.playback_multiplier += 1;
            }

            self.playback_multiplier = self.playback_multiplier.clamp(2, 10);
        } else {
            self.playback_multiplier = 1;
        }

        for _ in 1..self.playback_multiplier {
            self.core_update(game_io);
        }

        self.detect_debug_hotkeys(game_io);
        self.handle_exit_requests(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw simulation
        self.simulation.draw(
            game_io,
            &self.resources,
            render_pass,
            self.draw_player_indices,
        );

        // draw ui
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.ui_camera, SpriteColorMode::Multiply);

        // draw ui
        self.simulation.draw_ui(game_io, &mut sprite_queue);
        self.state.draw_ui(
            game_io,
            &self.resources,
            &mut self.simulation,
            &mut sprite_queue,
        );
        self.simulation.draw_hud_nodes(&mut sprite_queue);

        // draw ui fade color
        let fade_color = self.resources.ui_fade_color.get();
        self.resources
            .draw_fade_sprite(&mut sprite_queue, fade_color);

        #[cfg(not(feature = "record_every_frame"))]
        if self.playback_multiplier > 1 {
            let text = format!("{}X", self.playback_multiplier);
            let mut text_style = TextStyle::new_monospace(game_io, FontName::Code);
            text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;

            let metrics = text_style.measure(&text);
            text_style.bounds += RESOLUTION_F - metrics.size - 1.0;
            text_style.draw(game_io, &mut sprite_queue, &text);
        }

        if self.is_playing_back_recording {
            let globals = game_io.resource::<Globals>().unwrap();
            let assets = &globals.assets;

            let mut progress_sprite = assets.new_sprite(game_io, ResourcePaths::PIXEL);
            progress_sprite.set_color(Color::new(1.0, 1.0, 1.0, 0.5));

            let total_time = self.simulation.time + self.remaining_replay_buffer();
            let progress = self.simulation.time as f32 / total_time.max(1) as f32;
            progress_sprite.set_width(progress * RESOLUTION_F.x);

            progress_sprite.set_position(Vec2::new(0.0, RESOLUTION_F.y - 1.0));
            sprite_queue.draw_sprite(&progress_sprite);
        }

        // draw textbox over everything
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

const SMOOTH_FACTOR: f32 = 0.125;
const SMOOTH_FACTOR_FLIPPED: f32 = 1.0 - SMOOTH_FACTOR;

fn smooth_average_f32(old: f32, new: f32) -> f32 {
    old * SMOOTH_FACTOR_FLIPPED + new * SMOOTH_FACTOR
}
