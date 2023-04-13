use crate::battle::*;
use crate::bindable::SpriteColorMode;
use crate::lua_api::{create_battle_vm, encounter_init};
use crate::packages::{Package, PackageInfo};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use packets::NetplayPacket;
use std::collections::VecDeque;

const SLOW_COOLDOWN: FrameTime = INPUT_BUFFER_LIMIT as FrameTime;
const BUFFER_TOLERANCE: usize = 3;

#[derive(Default, Clone)]
struct PlayerController {
    connected: bool,
    input_buffer: VecDeque<Vec<Input>>,
}

struct Backup {
    simulation: BattleSimulation,
    state: Box<dyn State>,
}

pub struct BattleScene {
    battle_duration: FrameTime,
    ui_camera: Camera,
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
    started_music: bool,
    exiting: bool,
    statistics_callback: Option<BattleStatisticsCallback>,
    next_scene: NextScene,
}

impl BattleScene {
    pub fn new(game_io: &GameIO, mut props: BattleProps) -> Self {
        props.player_setups.sort_by_key(|setup| setup.index);

        let mut scene = Self {
            battle_duration: 0,
            ui_camera: Camera::new(game_io),
            synced_time: 0,
            shared_assets: SharedBattleAssets::new(game_io),
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
            started_music: false,
            exiting: false,
            statistics_callback: props.statistics_callback.take(),
            next_scene: NextScene::None,
        };

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
            if setup.local {
                scene.local_index = setup.index;
            }

            if let Some(remote_controller) = scene.player_controllers.get_mut(setup.index) {
                remote_controller.input_buffer = std::mem::take(&mut setup.input_buffer);
                remote_controller.connected = true;
            }

            // shuffle cards
            setup.deck.shuffle(&mut scene.simulation.rng);

            let result = scene.simulation.load_player(game_io, &scene.vms, setup);

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

        for package_info in dependencies {
            if package_info.package_category.requires_vm() {
                self.load_vm(game_io, package_info);
            }
        }
    }

    fn find_vm(&self, package_info: &PackageInfo) -> Option<usize> {
        self.vms
            .iter()
            .position(|vm| vm.path == package_info.script_path)
    }

    fn load_vm(&mut self, game_io: &GameIO, package_info: &PackageInfo) -> usize {
        let existing_vm = self.find_vm(package_info);

        if let Some(vm_index) = existing_vm {
            let vm = &mut self.vms[vm_index];

            if vm.namespace > package_info.namespace {
                // drop the vm to a lower namespace to make it more accessible
                vm.namespace = package_info.namespace;
            }

            if !vm.namespace.is_remote()
                && !package_info.namespace.is_remote()
                && vm.namespace != package_info.namespace
            {
                // vm already exists so no need to add a new one
                // exception for two different remotes, as with 3+ players recycling is more difficult
                return vm_index;
            }
        }

        create_battle_vm(game_io, &mut self.simulation, &mut self.vms, package_info)
    }

    fn is_solo(&self) -> bool {
        self.player_controllers.len() == 1
    }

    fn handle_packets(&mut self, game_io: &GameIO) {
        let mut packets = Vec::new();
        let mut pending_removal = Vec::new();

        let mut connected_count = self
            .player_controllers
            .iter()
            .enumerate()
            .filter(|(i, controller)| controller.connected && *i != self.local_index)
            .count();

        'main_loop: for (i, (index, receiver)) in self.receivers.iter().enumerate() {
            while let Ok(packet) = receiver.try_recv() {
                if index.is_some() && Some(packet.index()) != *index {
                    // ignore obvious impersonation cheat
                    continue;
                }

                let is_disconnect = matches!(packet, NetplayPacket::Disconnect { .. });

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
                if let Some(index) = index {
                    if let Some(controller) = self.player_controllers.get_mut(*index) {
                        controller.connected = false;
                    }
                }

                pending_removal.push(i);
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
        use num_traits::FromPrimitive;

        match packet {
            NetplayPacket::Input {
                index,
                pressed,
                buffer_sizes,
            } => {
                let mut resimulation_time = self.simulation.time;

                if let Some(controller) = self.player_controllers.get_mut(index) {
                    // figure out if we should slow down
                    if let Some(remote_received_size) = buffer_sizes.get(self.local_index).cloned()
                    {
                        let local_remote_size = controller.input_buffer.len();

                        let has_substantial_diff =
                            remote_received_size > local_remote_size + BUFFER_TOLERANCE;

                        if self.slow_cooldown == 0 && has_substantial_diff {
                            self.slow_cooldown = SLOW_COOLDOWN;
                        }
                    }

                    // convert input
                    let pressed: Vec<_> = pressed.into_iter().flat_map(Input::from_u8).collect();

                    if let Some(input) = self.simulation.inputs.get(index) {
                        if !input.matches(&pressed) {
                            // resolve the time of the input if it differs from our simulation
                            resimulation_time =
                                self.synced_time + controller.input_buffer.len() as FrameTime;
                        }
                    }

                    controller.input_buffer.push_back(pressed);
                    self.resimulate(game_io, resimulation_time);
                }
            }
            NetplayPacket::Disconnect { index } => {
                if let Some(controller) = self.player_controllers.get_mut(index) {
                    controller.connected = false;
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
            .all(|controller| !controller.connected || !controller.input_buffer.is_empty())
    }

    fn handle_local_input(&mut self, game_io: &GameIO) {
        if self.exiting {
            return;
        }

        let input_util = InputUtil::new(game_io);

        let local_controller = match self.player_controllers.get_mut(self.local_index) {
            Some(local_controller) => local_controller,
            None => return,
        };

        // gather input
        let mut pressed = Vec::new();

        for input in Input::BATTLE {
            if input_util.is_down(input) {
                pressed.push(input);
            }
        }

        let pressed_bytes: Vec<_> = pressed.iter().map(|input| *input as u8).collect();

        // update local buffer
        local_controller.input_buffer.push_back(pressed);

        // gather buffer sizes for remotes to know if they should slow down
        let buffer_sizes = self
            .player_controllers
            .iter()
            .map(|controller| controller.input_buffer.len())
            .collect();

        self.broadcast(NetplayPacket::Input {
            index: self.local_index,
            pressed: pressed_bytes,
            buffer_sizes,
        });
    }

    fn load_input(&mut self) {
        for (index, player_input) in self.simulation.inputs.iter_mut().enumerate() {
            player_input.flush();

            if let Some(controller) = self.player_controllers.get(index) {
                let index = (self.simulation.time - self.synced_time) as usize;

                if let Some(inputs) = controller.input_buffer.get(index) {
                    player_input.set_pressed_input(inputs.clone());
                }
            }
        }

        if self.input_synced() {
            use std::io::Write;

            let mut file = std::fs::OpenOptions::new()
                .append(true)
                .create(true)
                .open("_input_buffers.txt")
                .unwrap();

            let _ = writeln!(&mut file, "F: {}", self.synced_time);

            self.synced_time += 1;

            for controller in self.player_controllers.iter_mut() {
                let _ = writeln!(&mut file, "  {:?}", controller.input_buffer.front());
                controller.input_buffer.pop_front();
            }

            let _ = writeln!(&mut file, "Archetypes: [");

            for archetype in self.simulation.entities.archetypes() {
                let _ = write!(&mut file, "(");

                for t in archetype.component_types() {
                    let _ = write!(&mut file, "{t:?}, ");
                }

                let _ = write!(&mut file, "), ");
            }

            let _ = writeln!(&mut file, "]");
        }
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

    fn detect_exit_request(&self) -> bool {
        self.backups
            .front()
            .map(|backup| backup.simulation.exit)
            .unwrap_or(self.simulation.exit)
    }

    fn detect_debug_hotkeys(&self, game_io: &GameIO) {
        if !game_io.input().is_key_down(Key::F3) {
            return;
        }

        // list vms by memory usage
        if game_io.input().was_key_just_pressed(Key::V) {
            const NAMESPACE_WIDTH: usize = 9;
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
                "\n| Namespace | Package ID{:1$} | Used Memory | Free Memory |",
                " ",
                package_id_width - 10
            );

            // border
            println!("| {:-<NAMESPACE_WIDTH$} | {:-<package_id_width$} | {:-<MEMORY_WIDTH$} | {:-<MEMORY_WIDTH$} |", "", "", "", "");

            // list
            for vm in vms_sorted {
                println!(
                    "| {:NAMESPACE_WIDTH$} | {:package_id_width$} | {:MEMORY_WIDTH$} | {:MEMORY_WIDTH$} |",
                    format!("{:?}", vm.namespace),
                    vm.package_id,
                    vm.lua.used_memory(),
                    vm.lua.unused_memory().unwrap()
                );
            }

            // margin
            println!();
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
        self.detect_debug_hotkeys(game_io);

        if !game_io.is_in_transition() && !self.started_music {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_music(&globals.music.battle, true);

            self.started_music = true;
        }

        self.handle_packets(game_io);

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

        self.simulation.camera.update(game_io);

        if !self.exiting && self.detect_exit_request() {
            self.exiting = true;
            self.broadcast(NetplayPacket::Disconnect {
                index: self.local_index,
            });

            if let Some(statistics_callback) = self.statistics_callback.take() {
                self.simulation.wrap_up_statistics();
                statistics_callback(Some(self.simulation.statistics.clone()));
            }

            let transition = crate::transitions::new_battle_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);

            // clean up music stack
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.pop_music_stack();
            globals.audio.restart_music();
        }
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

        render_pass.consume_queue(sprite_queue);
    }
}
