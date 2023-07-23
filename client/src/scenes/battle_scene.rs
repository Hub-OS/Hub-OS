use crate::battle::*;
use crate::bindable::SpriteColorMode;
use crate::lua_api::{create_battle_vm, encounter_init};
use crate::packages::{Package, PackageInfo, PackageNamespace};
use crate::render::ui::{Textbox, TextboxMessage, TextboxQuestion};
use crate::render::*;
use crate::resources::*;
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
    buffer: VecDeque<NetplayBufferItem>,
}

struct Backup {
    simulation: BattleSimulation,
    state: Box<dyn State>,
}

pub struct BattleScene {
    battle_duration: FrameTime,
    ui_camera: Camera,
    textbox: Textbox,
    textbox_is_blocking_input: bool,
    event_receiver: flume::Receiver<BattleEvent>,
    pending_signals: Vec<NetplaySignal>,
    synced_time: FrameTime,
    shared_assets: SharedBattleAssets,
    simulation: BattleSimulation,
    state: Box<dyn State>,
    backups: VecDeque<Backup>,
    vms: Vec<RollbackVM>,
    player_controllers: Vec<PlayerController>,
    local_index: usize,
    senders: Vec<NetplayPacketSender>,
    receivers: Vec<(Option<usize>, NetplayPacketReceiver)>,
    slow_cooldown: FrameTime,
    frame_by_frame_debug: bool,
    already_snapped: bool,
    exiting: bool,
    statistics_callback: Option<BattleStatisticsCallback>,
    next_scene: NextScene,
}

impl BattleScene {
    pub fn new(game_io: &GameIO, mut props: BattleProps) -> Self {
        props.player_setups.sort_by_key(|setup| setup.index);

        let (event_sender, event_receiver) = flume::unbounded();

        let mut scene = Self {
            battle_duration: 0,
            ui_camera: Camera::new(game_io),
            textbox: Textbox::new_overworld(game_io),
            textbox_is_blocking_input: false,
            event_receiver,
            pending_signals: Vec::new(),
            synced_time: 0,
            shared_assets: SharedBattleAssets::new(game_io, event_sender),
            simulation: BattleSimulation::new(
                game_io,
                props.background.clone(),
                props.player_setups.len(),
            ),
            state: Box::new(IntroState::new()),
            backups: VecDeque::new(),
            vms: Vec::new(),
            player_controllers: vec![PlayerController::default(); props.player_setups.len()],
            local_index: 0,
            senders: std::mem::take(&mut props.senders),
            receivers: std::mem::take(&mut props.receivers),
            slow_cooldown: 0,
            frame_by_frame_debug: false,
            already_snapped: false,
            exiting: false,
            statistics_callback: props.statistics_callback.take(),
            next_scene: NextScene::None,
        };

        if let Some(setup) = props.player_setups.iter().find(|setup| setup.local) {
            // resolve local index early for namespace remapping in vm creation
            scene.local_index = setup.index;
        }

        scene.ui_camera.snap(RESOLUTION_F * 0.5);

        // seed before running any vm
        if let Some(seed) = props.seed {
            scene.simulation.seed_random(seed);
        }

        // load every vm we need
        scene.load_vms(game_io, &props);

        // load battle package
        if let Some(encounter_package) = props.encounter_package {
            let vm_index = scene.find_vm(encounter_package.package_info()).unwrap();

            let context = BattleScriptContext {
                vm_index,
                vms: &scene.vms,
                game_io,
                simulation: &mut scene.simulation,
            };

            encounter_init(context, props.data);
        }

        // load the players in the correct order
        for mut setup in props.player_setups {
            if let Some(remote_controller) = scene.player_controllers.get_mut(setup.index) {
                remote_controller.buffer = std::mem::take(&mut setup.buffer);
                remote_controller.connected = true;
            }

            // shuffle cards
            let rng = &mut scene.simulation.rng;
            setup.deck.shuffle(game_io, rng, setup.namespace());

            let result = Player::load(game_io, &mut scene.simulation, &scene.vms, setup);

            if let Err(e) = result {
                log::error!("{e}");
            }
        }

        scene.simulation.initialize_uninitialized();

        scene
    }

    fn load_vms(&mut self, game_io: &GameIO, props: &BattleProps) {
        let globals = game_io.resource::<Globals>().unwrap();
        let dependencies = globals.battle_dependencies(game_io, props);

        for (package_info, namespace) in dependencies {
            if package_info.package_category.requires_vm() {
                self.load_vm(game_io, package_info, namespace);
            }
        }
    }

    fn find_vm(&self, package_info: &PackageInfo) -> Option<usize> {
        self.vms
            .iter()
            .position(|vm| vm.path == package_info.script_path)
    }

    fn load_vm(
        &mut self,
        game_io: &GameIO,
        package_info: &PackageInfo,
        namespace: PackageNamespace,
    ) -> usize {
        // expecting local namespace to be converted to a Netplay namespace
        // before being passed to this function
        assert_ne!(namespace, PackageNamespace::Local);

        let existing_vm = self.find_vm(package_info);

        if let Some(vm_index) = existing_vm {
            // recycle a vm
            let vm = &mut self.vms[vm_index];

            if !vm.namespaces.contains(&namespace) {
                vm.namespaces.push(namespace);
            }

            return vm_index;
        }

        // new vm
        create_battle_vm(
            game_io,
            &mut self.simulation,
            &mut self.vms,
            package_info,
            namespace,
        )
    }

    fn is_solo(&self) -> bool {
        self.player_controllers.len() == 1
    }

    fn update_textbox(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
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
                        let event_sender = self.shared_assets.event_sender.clone();
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
                    let event_sender = self.shared_assets.event_sender.clone();
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
            .filter(|(i, controller)| controller.connected && *i != self.local_index)
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
                    if let Some(remote_received_size) = buffer_sizes.get(self.local_index).cloned()
                    {
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

                    controller.buffer.push_back(data);
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
        let input_util = InputUtil::new(game_io);

        let local_controller = match self.player_controllers.get_mut(self.local_index) {
            Some(local_controller) => local_controller,
            None => return,
        };

        // gather input
        let mut pressed = Vec::new();

        if self.exiting || !self.textbox_is_blocking_input {
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
        local_controller.buffer.push_back(data.clone());

        // gather buffer sizes for remotes to know if they should slow down
        let buffer_sizes = self
            .player_controllers
            .iter()
            .map(|controller| controller.buffer.len())
            .collect();

        self.broadcast(NetplayPacket::Buffer {
            index: self.local_index,
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

        if self.input_synced() {
            // log inputs if we're in multiplayer to help track desyncs
            if self.player_controllers.len() > 1 {
                if let Err(e) = self.log_input_to_file() {
                    log::error!("{e:?}");
                }
            }

            // prevent buffers from infinitely growing
            for controller in self.player_controllers.iter_mut() {
                controller.buffer.pop_front();
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
            writeln!(&mut file, "  {:?}", controller.buffer.front())?;
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
        self.synced_time = self.simulation.time;
    }

    fn rollback(&mut self, game_io: &GameIO, steps: usize) {
        for vm in &mut self.vms {
            vm.lua.rollback(steps);
        }

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
            for vm in &mut self.vms {
                vm.lua.snap();
            }

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

        self.simulation
            .handle_local_signals(self.local_index, &mut self.shared_assets);

        self.simulation.update_background();
        self.simulation.pre_update(game_io, &self.vms, &*self.state);
        self.state.update(
            game_io,
            &mut self.shared_assets,
            &mut self.simulation,
            &self.vms,
        );
        self.simulation.post_update(game_io, &self.vms);

        if self.backups.len() > INPUT_BUFFER_LIMIT {
            self.backups.pop_front();
        }
    }

    fn detect_debug_hotkeys(&self, game_io: &GameIO) {
        if !game_io.input().is_key_down(Key::F3) {
            return;
        }

        // list vms by memory usage
        if game_io.input().was_key_just_pressed(Key::V) {
            const NAMESPACE_WIDTH: usize = 10;
            const MEMORY_WIDTH: usize = 11;

            let mut vms_sorted: Vec<_> = self.vms.iter().collect();
            vms_sorted.sort_by_key(|vm| vm.lua.unused_memory());

            let package_id_width = vms_sorted
                .iter()
                .map(|vm| vm.package_id.as_str().len())
                .max()
                .unwrap_or_default()
                .max(10);

            // margin + header
            println!(
                "\n| Namespace  | Package ID{:1$} | Used Memory | Free Memory |",
                " ",
                package_id_width - 10
            );

            // border
            println!("| {:-<NAMESPACE_WIDTH$} | {:-<package_id_width$} | {:-<MEMORY_WIDTH$} | {:-<MEMORY_WIDTH$} |", "", "", "", "");

            // list
            for vm in vms_sorted {
                println!(
                    "| {:NAMESPACE_WIDTH$} | {:package_id_width$} | {:MEMORY_WIDTH$} | {:MEMORY_WIDTH$} |",
                    // todo: maybe we should list every namespace in a compressed format
                    // ex: [Server, Local, Netplay(0)] -> S L 0
                    format!("{:?}", vm.preferred_namespace()),
                    vm.package_id,
                    vm.lua.used_memory(),
                    vm.lua.unused_memory().unwrap()
                );
            }

            // margin
            println!();
        }
    }

    fn exit(&mut self, game_io: &GameIO, fleeing: bool) {
        self.exiting = true;
        self.pending_signals.push(NetplaySignal::Disconnect);

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

    fn core_update(&mut self, game_io: &mut GameIO) {
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
            let can_buffer =
                self.simulation.time < self.synced_time + INPUT_BUFFER_LIMIT as FrameTime;
            let should_slow_down = self.slow_cooldown == SLOW_COOLDOWN;

            if !should_slow_down && (can_buffer || self.input_synced()) {
                self.handle_local_input(game_io);
                self.simulate(game_io);
            }

            if self.slow_cooldown > 0 {
                self.slow_cooldown -= 1;
            }

            self.frame_by_frame_debug = self.is_solo()
                && (input_util.was_just_pressed(Input::RewindFrame)
                    || input_util.was_just_pressed(Input::AdvanceFrame));
        }
    }

    fn handle_exit_requests(&mut self, game_io: &GameIO) {
        let oldest_backup = self.backups.front();

        let requested_exit = oldest_backup
            .map(|backup| backup.simulation.exit)
            .unwrap_or(self.simulation.exit);

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
