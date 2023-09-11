use crate::battle::*;
use crate::bindable::SpriteColorMode;
use crate::lua_api::encounter_init;
use crate::packages::{Package, PackageNamespace};
use crate::render::ui::{Textbox, TextboxMessage, TextboxQuestion};
use crate::render::*;
use crate::resources::*;
use crate::saves::{BattleRecording, PlayerInputBuffer};
use framework::prelude::*;
use packets::structures::PackageId;
use packets::{NetplayBufferItem, NetplayPacket, NetplaySignal};
use std::collections::VecDeque;

const SLOW_COOLDOWN: FrameTime = INPUT_BUFFER_LIMIT as FrameTime;
const BUFFER_TOLERANCE: usize = 3;

pub enum BattleEvent {
    Description(String),
    DescribeCard(PackageId),
    RequestFlee,
    AttemptFlee,
    FleeResult(bool),
    CompleteFlee(bool),
}

#[derive(Default, Clone)]
struct PlayerController {
    connected: bool,
    buffer: PlayerInputBuffer,
}

struct Backup {
    simulation: BattleSimulation,
    state: Box<dyn State>,
}

pub struct BattleScene {
    battle_duration: FrameTime,
    recording: Option<BattleRecording>,
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
    local_index: Option<usize>,
    senders: Vec<NetplayPacketSender>,
    receivers: Vec<(Option<usize>, NetplayPacketReceiver)>,
    slow_cooldown: FrameTime,
    frame_by_frame_debug: bool,
    already_snapped: bool,
    is_playing_back_recording: bool,
    exiting: bool,
    statistics_callback: Option<BattleStatisticsCallback>,
    next_scene: NextScene,
}

impl BattleScene {
    pub fn new(game_io: &mut GameIO, mut props: BattleProps) -> Self {
        let mut is_playing_back_recording = false;

        if let Some(recording) = props.recording_data(game_io) {
            recording.load_packages(game_io);
            props = BattleProps::from_recording(game_io, &recording);
            is_playing_back_recording = true;
        } else {
            // remove recording namespaces to prevent interference with other namespaces
            // recording namespaces have precedence over other namespaces
            let globals = game_io.resource_mut::<Globals>().unwrap();
            globals.remove_namespace(PackageNamespace::RecordingServer);
            globals.assets.remove_unused_virtual_zips();
        }

        // sort player setups for consistent execution order on every client
        props.player_setups.sort_by_key(|setup| setup.index);

        // init recording struct
        let recording = if props.recording_enabled {
            Some(BattleRecording::new(game_io, &props))
        } else {
            None
        };

        // resolve dependencies for loading vms
        let globals = game_io.resource::<Globals>().unwrap();
        let dependencies = globals.battle_dependencies(game_io, &props);

        // create initial simulation
        let mut simulation = BattleSimulation::new(game_io, &props);

        // seed before running any vm
        simulation.seed_random(props.seed);

        // create shared resources
        let mut resources = SharedBattleResources::new(game_io, &mut simulation, &dependencies);

        // load battle package
        if let Some(encounter_package) = props.encounter_package(game_io) {
            let vm_manager = &mut resources.vm_manager;
            let vm_index = vm_manager
                .find_vm_from_info(encounter_package.package_info())
                .unwrap();

            let context = BattleScriptContext {
                vm_index,
                resources: &mut resources,
                game_io,
                simulation: &mut simulation,
            };

            encounter_init(context, props.data);
        }

        // load the players in the correct order
        let player_setups = props.player_setups;
        let mut player_controllers = vec![PlayerController::default(); player_setups.len()];
        let local_index = if is_playing_back_recording {
            None
        } else {
            player_setups
                .iter()
                .find(|setup| setup.local)
                .map(|setup| setup.index)
        };

        for mut setup in player_setups {
            if let Some(remote_controller) = player_controllers.get_mut(setup.index) {
                remote_controller.buffer = std::mem::take(&mut setup.buffer);
                remote_controller.connected = true;
            }

            // shuffle cards
            let rng = &mut simulation.rng;
            setup.deck.shuffle(game_io, rng, setup.namespace());

            let result = Player::load(game_io, &resources, &mut simulation, setup);

            if let Err(e) = result {
                log::error!("{e}");
            }
        }

        simulation.initialize_uninitialized();

        Self {
            battle_duration: 0,
            recording,
            ui_camera: Camera::new_ui(game_io),
            textbox: Textbox::new_overworld(game_io),
            textbox_is_blocking_input: false,
            pending_signals: Vec::new(),
            synced_time: 0,
            resources,
            simulation,
            state: Box::new(IntroState::new()),
            backups: VecDeque::new(),
            player_controllers,
            local_index,
            senders: std::mem::take(&mut props.senders),
            receivers: std::mem::take(&mut props.receivers),
            slow_cooldown: 0,
            frame_by_frame_debug: false,
            already_snapped: false,
            is_playing_back_recording,
            exiting: false,
            statistics_callback: props.statistics_callback.take(),
            next_scene: NextScene::None,
        }
    }

    fn is_solo(&self) -> bool {
        self.player_controllers.len() == 1
    }

    fn update_textbox(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.resources.event_receiver.try_recv() {
            match event {
                BattleEvent::Description(description) => {
                    if !description.is_empty() {
                        let interface = TextboxMessage::new(description);
                        self.textbox.push_interface(interface);
                        self.textbox.open();
                    }
                }
                BattleEvent::DescribeCard(package_id) => {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let ns = PackageNamespace::Local;

                    let Some(package) = globals.card_packages.package_or_override(ns, &package_id)
                    else {
                        continue;
                    };

                    let description = if package.long_description.is_empty() {
                        package.description.clone()
                    } else {
                        package.long_description.clone()
                    };

                    self.textbox.use_blank_avatar(game_io);
                    let interface = TextboxMessage::new(description);
                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
                BattleEvent::RequestFlee => {
                    let is_boss_battle = self.simulation.statistics.boss_battle;
                    let can_run = !is_boss_battle;

                    self.textbox.use_player_avatar(game_io);

                    if can_run {
                        // confirm
                        let event_sender = self.resources.event_sender.clone();
                        let text = String::from("Should we run?");

                        let interface = TextboxQuestion::new(text, move |yes| {
                            if yes {
                                event_sender.send(BattleEvent::AttemptFlee).unwrap();
                            }
                        });

                        self.textbox.push_interface(interface);
                    } else {
                        // can't run
                        let text = String::from("This is no time to run!");
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
                    let text = if success {
                        String::from("\x01...\x01OK!\nWe got away!")
                    } else {
                        String::from("\x01...\x01It's no good!\nWe can't run away!")
                    };

                    let interface = TextboxMessage::new(text).with_callback(move || {
                        event_sender
                            .send(BattleEvent::CompleteFlee(success))
                            .unwrap();
                    });

                    self.textbox.use_player_avatar(game_io);
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

    fn count_connected_players(&self) -> usize {
        self.player_controllers
            .iter()
            .enumerate()
            .filter(|(i, controller)| controller.connected && Some(*i) != self.local_index)
            .count()
    }

    fn handle_packets(&mut self, game_io: &GameIO) {
        let mut packets = Vec::new();
        let mut pending_removal = Vec::new();

        let mut connected_count = self.count_connected_players();

        'main_loop: for (i, (index, receiver)) in self.receivers.iter().enumerate() {
            while let Ok(packet) = receiver.try_recv() {
                if index.is_some() && Some(packet.index()) != *index {
                    // ignore obvious impersonation cheat
                    continue;
                }

                let is_disconnect = matches!(
                    &packet,
                    NetplayPacket::Buffer { data, .. } if data.signals.contains(&NetplaySignal::Disconnect)
                );

                packets.push(packet);

                if is_disconnect {
                    connected_count -= 1;

                    if connected_count == 0 {
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
        for i in pending_removal.into_iter().rev() {
            let (player_index, _) = self.receivers.remove(i);

            let Some(index) = player_index else {
                continue;
            };

            let Some(controller) = self.player_controllers.get_mut(index) else {
                continue;
            };

            if controller.connected {
                // possible desync when there's another player we need to sync a disconnect with
                log::error!("Possible desync from a player disconnect without a Disconnect signal");

                packets.push(NetplayPacket::new_disconnect_signal(index));
            }
        }

        if connected_count == 0 {
            // no need to store these, helps prevent reading too many packets from the fallback receiver
            self.receivers.clear();
        }

        for packet in packets {
            self.handle_packet(game_io, packet);
        }
    }

    fn handle_packet(&mut self, game_io: &GameIO, packet: NetplayPacket) {
        match packet {
            NetplayPacket::Buffer {
                index,
                data,
                buffer_sizes,
            } => {
                let mut resimulation_time = self.simulation.time;

                if let Some(controller) = self.player_controllers.get_mut(index) {
                    // check disconnect
                    if data.signals.contains(&NetplaySignal::Disconnect) {
                        controller.connected = false;
                    }

                    // figure out if we should slow down
                    let remote_received_size = self
                        .local_index
                        .and_then(|index| buffer_sizes.get(index))
                        .cloned();

                    if let Some(remote_received_size) = remote_received_size {
                        let local_remote_size = controller.buffer.len();

                        let has_substantial_diff = controller.connected
                            && remote_received_size > local_remote_size + BUFFER_TOLERANCE;

                        if self.slow_cooldown == 0 && has_substantial_diff {
                            self.slow_cooldown = SLOW_COOLDOWN;
                        }
                    }

                    if let Some(input) = self.simulation.inputs.get(index) {
                        if !input.matches(&data) {
                            // resolve the time of the input if it differs from our simulation
                            resimulation_time =
                                self.synced_time + controller.buffer.len() as FrameTime;
                        }
                    }

                    if !data.signals.is_empty() {
                        log::debug!("Received {:?} from {index}", data.signals);
                    }

                    controller.buffer.push_last(data);
                    self.resimulate(game_io, resimulation_time);
                }
            }
            NetplayPacket::Heartbeat { .. } => {}
            packet => {
                let name: &'static str = (&packet).into();
                let index = packet.index();

                log::error!("Expecting Input, Heartbeat, or Disconnect during battle, received: {name} from {index}");
            }
        }
    }

    fn broadcast(&self, packet: NetplayPacket) {
        for send in &self.senders {
            send(packet.clone());
        }
    }

    fn input_synced(&self) -> bool {
        self.player_controllers
            .iter()
            .all(|controller| !controller.connected || !controller.buffer.is_empty())
    }

    fn handle_local_input(&mut self, game_io: &GameIO) {
        let Some(local_index) = self.local_index else {
            return;
        };

        let input_util = InputUtil::new(game_io);

        let local_controller = match self.player_controllers.get_mut(local_index) {
            Some(local_controller) => local_controller,
            None => return,
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

        // gather buffer sizes for remotes to know if they should slow down
        let buffer_sizes = self
            .player_controllers
            .iter()
            .map(|controller| controller.buffer.len())
            .collect();

        self.broadcast(NetplayPacket::Buffer {
            index: local_index,
            data,
            buffer_sizes,
        });
    }

    fn load_input(&mut self) {
        for (index, player_input) in self.simulation.inputs.iter_mut().enumerate() {
            player_input.flush();

            if let Some(controller) = self.player_controllers.get(index) {
                let index = (self.simulation.time - self.synced_time) as usize;

                if let Some(data) = controller.buffer.get(index) {
                    player_input.load_data(data.clone());
                }
            }
        }

        if self.input_synced() && !self.is_playing_back_recording {
            // log inputs if we're in multiplayer to help track desyncs
            if self.player_controllers.len() > 1 {
                if let Err(e) = self.log_input_to_file() {
                    log::error!("{e:?}");
                }
            }

            // prevent buffers from infinitely growing
            for (i, controller) in self.player_controllers.iter_mut().enumerate() {
                let buffer_item = controller.buffer.pop_next().unwrap();

                // record input
                if let Some(recording) = &mut self.recording {
                    recording.player_setups[i].buffer.push_last(buffer_item);
                }
            }

            self.synced_time += 1;
        }
    }

    fn log_input_to_file(&self) -> std::io::Result<()> {
        use std::io::Write;

        let mut file = std::fs::OpenOptions::new()
            .append(true)
            .create(true)
            .open("_input_buffers.txt")?;

        writeln!(&mut file, "F: {}", self.synced_time)?;

        for controller in &self.player_controllers {
            writeln!(&mut file, "  {:?}", controller.buffer.peek_next())?;
        }

        writeln!(&mut file, "Archetypes: [")?;

        for archetype in self.simulation.entities.archetypes() {
            write!(&mut file, "(")?;

            for t in archetype.component_types() {
                write!(&mut file, "{t:?}, ")?;
            }

            write!(&mut file, "), ")?;
        }

        writeln!(&mut file, "]")?;

        Ok(())
    }

    fn resimulate(&mut self, game_io: &GameIO, start_time: FrameTime) {
        let local_time = self.simulation.time;

        if start_time >= local_time {
            // no need to resimulate, we're behind
            return;
        }

        // rollback
        let steps = (local_time - start_time) as usize;
        self.rollback(game_io, steps);

        self.simulation.is_resimulation = true;

        // resimulate until we're caught up to our previous time
        while self.simulation.time < local_time {
            // avoiding snapshots for the first frame as it's still retained
            self.simulate(game_io);
        }

        self.simulation.is_resimulation = false;
    }

    fn rewind(&mut self, game_io: &GameIO, mut steps: usize) {
        // taking an extra step back as we'll resimulate one step forward again
        // this ensures the snapshot is popped as repeated self.rollback(1) has no effect
        steps += 1;

        if self.backups.len() < steps {
            return;
        }

        self.rollback(game_io, steps);
        self.simulate(game_io);

        if !self.is_playing_back_recording {
            // only updating synced time for actual battles
            // synced_time is used for input buffer offsets, which playback doesn't delete
            self.synced_time = self.simulation.time;
        }

        // undo input committed to the recording
        if let Some(recording) = &mut self.recording {
            for setup in &mut recording.player_setups {
                for _ in 0..steps - 1 {
                    setup.buffer.delete_last();
                }

                debug_assert_eq!(self.synced_time as usize, setup.buffer.len());
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

    fn simulate(&mut self, game_io: &GameIO) {
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

        simulation.update_background();
        simulation.pre_update(game_io, resources, state);
        state.update(game_io, resources, simulation);
        simulation.post_update(game_io, resources);

        if self.backups.len() > INPUT_BUFFER_LIMIT {
            self.backups.pop_front();
        }
    }

    fn detect_debug_hotkeys(&self, game_io: &mut GameIO) {
        if !game_io.input().is_key_down(Key::F3) {
            return;
        }

        // list vms by memory usage
        if game_io.input().was_key_just_pressed(Key::V) {
            self.resources.vm_manager.print_memory_usage();
        }

        // save recording
        if game_io.input().was_key_just_pressed(Key::S) {
            if let Some(recording) = &self.recording {
                recording.save(game_io);
            } else {
                log::error!("Recording is disabled");
            }
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

    fn core_update(&mut self, game_io: &GameIO) {
        if game_io.is_in_transition() {
            self.simulation.update_background();
            return;
        }

        let input_util = InputUtil::new(game_io);

        if self.frame_by_frame_debug {
            let rewind = input_util.was_just_pressed(Input::RewindFrame);

            if rewind {
                self.rewind(game_io, 1);
            }

            let advance = input_util.was_just_pressed(Input::AdvanceFrame);

            if advance {
                self.handle_local_input(game_io);
                self.simulate(game_io);
            }

            // exit from frame_by_frame_debug with pause
            self.frame_by_frame_debug = !input_util.was_just_pressed(Input::Pause);
        } else {
            // normal update
            let can_simulate = if self.is_playing_back_recording {
                // simulate as long as we have input
                self.simulation.time < self.player_controllers[0].buffer.len() as FrameTime
            } else {
                // simulate as long as we can roll back to the synced time
                self.simulation.time < self.synced_time + INPUT_BUFFER_LIMIT as FrameTime
                    || self.input_synced()
            };
            let should_slow_down = self.slow_cooldown == SLOW_COOLDOWN;

            if !should_slow_down && can_simulate {
                self.handle_local_input(game_io);
                self.simulate(game_io);
            }

            if self.slow_cooldown > 0 {
                self.slow_cooldown -= 1;
            }

            self.frame_by_frame_debug = (self.is_playing_back_recording || self.is_solo())
                && (input_util.was_just_pressed(Input::RewindFrame)
                    || input_util.was_just_pressed(Input::AdvanceFrame));
        }
    }

    fn handle_exit_requests(&mut self, game_io: &GameIO) {
        let requested_exit = if self.is_playing_back_recording {
            // pressing confirm or cancel, without pressing pause
            // as pause is used to exit frame_by_frame_debug
            // and the same input may also be binded to Confirm or Cancel
            let input_util = InputUtil::new(game_io);

            !input_util.was_just_pressed(Input::Pause)
                && (input_util.was_just_pressed(Input::Cancel)
                    || input_util.was_just_pressed(Input::Confirm))
        } else {
            // use the oldest backup, as we can still rewind and end up not exitting otherwise
            let oldest_backup = self.backups.front();

            oldest_backup
                .map(|backup| backup.simulation.exit)
                .unwrap_or(self.simulation.exit)
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

    fn update(&mut self, game_io: &mut GameIO) {
        self.update_textbox(game_io);
        self.handle_packets(game_io);
        self.core_update(game_io);
        self.detect_debug_hotkeys(game_io);
        self.handle_exit_requests(game_io);
        self.simulation.camera.update(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw simulation
        self.simulation.draw(game_io, render_pass);

        // draw ui
        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.ui_camera, SpriteColorMode::Multiply);

        self.simulation.draw_ui(game_io, &mut sprite_queue);
        self.state
            .draw_ui(game_io, &mut self.simulation, &mut sprite_queue);

        // draw textbox over everything
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
