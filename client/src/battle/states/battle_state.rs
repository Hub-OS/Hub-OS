use super::State;
use crate::battle::*;
use crate::bindable::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::borrow::Cow;
use ui::BattleBannerMessage;

const TOTAL_MESSAGE_TIME: FrameTime = 3 * 60;

#[derive(Clone)]
pub struct BattleState {
    time: FrameTime,
    end_message: BattleBannerMessage,
    complete: bool,
    out_of_time: bool,
}

impl State for BattleState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, game_io: &GameIO) -> Option<Box<dyn State>> {
        if self.out_of_time {
            Some(Box::new(TimeUpState::new()))
        } else if self.complete {
            Some(Box::new(CardSelectState::new(game_io)))
        } else {
            None
        }
    }

    fn allows_animation_updates(&self) -> bool {
        true
    }

    fn update(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        self.end_message.update();

        self.detect_battle_start(game_io, resources, simulation);

        // reset frame temporary variables
        self.prepare_updates(simulation);

        // allow cards to mutate
        Character::mutate_cards(game_io, resources, simulation);

        // new: process player input
        self.process_input(game_io, resources, simulation);

        Character::process_card_requests(game_io, resources, simulation, self.time);

        Action::process_queues(game_io, resources, simulation);

        // update time freeze first as it affects the rest of the updates
        TimeFreezeTracker::update(game_io, resources, simulation);

        // new: process movement and actions
        self.process_movement(game_io, resources, simulation);
        Action::process_actions(game_io, resources, simulation);

        // update tiles
        self.update_field(game_io, resources, simulation);

        // update spells
        self.update_spells(game_io, resources, simulation);

        // todo: handle spells attacking on tiles they're not on?
        // this can unintentionally occur by queueing an attack outside of update_spells,
        // such as in a Battle component or callback

        // execute attacks
        self.execute_attacks(game_io, resources, simulation);

        // process 0 HP
        self.mark_deleted(game_io, resources, simulation);

        // new: update living, processes statuses
        self.update_living(game_io, resources, simulation);

        // update artifacts
        self.update_artifacts(game_io, resources, simulation);

        if !simulation.time_freeze_tracker.time_is_frozen() {
            // update active battle components
            simulation.update_components(game_io, resources, ComponentLifetime::ActiveBattle);
        }

        // update battle components
        simulation.update_components(game_io, resources, ComponentLifetime::Battle);

        simulation.call_pending_callbacks(game_io, resources);

        self.apply_status_vfx(game_io, resources, simulation);

        if self.end_message.remaining_time().is_none()
            && !simulation.time_freeze_tracker.time_is_frozen()
        {
            // only update the time statistic if the battle is still going for the local player
            // and if time is not frozen
            simulation.statistics.time += 1;
        }

        // other players may still be in battle, and some components make use of this
        simulation.battle_time += 1;
        self.time += 1;

        self.detect_success_or_failure(simulation);
        self.update_turn_gauge(game_io, simulation);
        self.play_low_hp_sfx(game_io, simulation);
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        // win / lose message
        if self.end_message.remaining_time().is_some() {
            self.end_message.draw(game_io, sprite_queue);
        } else {
            // turn gauge
            simulation.turn_gauge.draw(sprite_queue);
        }

        // time freeze
        let time_freeze_tracker = &simulation.time_freeze_tracker;
        time_freeze_tracker.draw_ui(game_io, resources, simulation, sprite_queue);

        // trap indicators
        DefenseRule::draw_trap_ui(game_io, simulation, sprite_queue);

        Character::draw_top_card_ui(
            game_io,
            simulation,
            simulation.local_player_id,
            sprite_queue,
        );
    }
}

impl BattleState {
    pub const GRACE_TIME: FrameTime = 5;

    pub fn new() -> Self {
        Self {
            time: 0,
            end_message: BattleBannerMessage::default(),
            complete: false,
            out_of_time: false,
        }
    }

    fn play_low_hp_sfx(&self, game_io: &GameIO, simulation: &mut BattleSimulation) {
        if !simulation.local_health_ui.is_low_hp() || self.time % LOW_HP_SFX_RATE != 0 {
            return;
        };

        let entities = &mut simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&Entity>(simulation.local_player_id.into())
        else {
            return;
        };

        if entity.deleted {
            return;
        }

        let globals = game_io.resource::<Globals>().unwrap();
        let audio = &globals.audio;
        let sfx = &globals.sfx.low_hp;

        audio.play_sound_with_behavior(sfx, AudioBehavior::NoOverlap);
    }

    fn update_turn_gauge(&mut self, game_io: &GameIO, simulation: &mut BattleSimulation) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            return;
        }

        let previously_incomplete = !simulation.turn_gauge.is_complete();

        simulation.turn_gauge.increment_time();

        if !simulation.turn_gauge.is_complete() || self.end_message.remaining_time().is_some() {
            // don't check for input if the battle has ended, or if the turn guage isn't complete
            return;
        }

        if previously_incomplete {
            // just completed, play a sfx
            let globals = game_io.resource::<Globals>().unwrap();
            simulation.play_sound(game_io, &globals.sfx.turn_gauge);
        }

        if simulation.config.turn_limit == Some(simulation.statistics.turns) {
            self.out_of_time = true;
            return;
        }

        if simulation.config.automatic_turn_end || simulation.turn_gauge.completed_turn() {
            simulation.turn_gauge.set_completed_turn(false);
            self.complete = true;
            return;
        }

        let mut player_iter = simulation
            .entities
            .query_mut::<(&Entity, &mut Player)>()
            .into_iter();

        self.complete = player_iter.any(|(_, (entity, player))| {
            if entity.deleted {
                return false;
            }

            let input = &simulation.inputs[player.index];
            input.was_just_pressed(Input::EndTurn)
        });
    }

    fn detect_success_or_failure(&mut self, simulation: &mut BattleSimulation) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            // allow the time freeze action to finish
            return;
        }

        if let Some(t) = self.end_message.remaining_time() {
            if t == 0 {
                simulation.exit = true;
            }
            return;
        }

        // detect failure + find team for success detection
        let local_team;

        if let Ok(entity) =
            (simulation.entities).query_one_mut::<&Entity>(simulation.local_player_id.into())
        {
            if entity.deleted {
                self.fail();
                return;
            }

            local_team = entity.team;
        } else {
            self.fail();
            return;
        }

        // detect success
        // todo: score screen

        let enemies_alive = simulation
            .entities
            .query_mut::<(&Entity, &Character)>()
            .into_iter()
            .any(|(_, (entity, _))| entity.team != local_team);

        if !enemies_alive {
            self.succeed(simulation);
        }
    }

    fn fail(&mut self) {
        self.end_message.set_message(Cow::Borrowed("<_FAILED_>"));
        self.end_message.show_for(TOTAL_MESSAGE_TIME);
    }

    fn succeed(&mut self, simulation: &mut BattleSimulation) {
        self.end_message.set_message(Cow::Borrowed("<_SUCCESS_>"));
        self.end_message.show_for(TOTAL_MESSAGE_TIME);
        simulation.statistics.won = true;
    }

    fn detect_battle_start(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        if simulation.battle_started {
            return;
        }

        simulation.battle_started = true;

        for (_, entity) in simulation.entities.query_mut::<&mut Entity>() {
            let callback = entity.battle_start_callback.clone();
            simulation.pending_callbacks.push(callback);
        }

        simulation.call_pending_callbacks(game_io, resources);
    }

    fn prepare_updates(&self, simulation: &mut BattleSimulation) {
        // entities should only update once, clearing the flag that tracks this
        for (_, entity) in simulation.entities.query_mut::<&mut Entity>() {
            entity.updated = false;

            // reset frame temp properties
            entity.movement_offset = Vec2::ZERO;

            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
                let sprite_node = sprite_tree.root_mut();
                sprite_node.set_color(Color::BLACK);
                sprite_node.set_color_mode(SpriteColorMode::Add);
            }
        }
    }

    pub fn update_field(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        simulation.field.reset_highlight();

        if simulation.time_freeze_tracker.time_is_frozen() {
            // skip tile effect processing if time is frozen
            return;
        }

        simulation.field.update_tiles();
        Field::apply_side_effects(game_io, resources, simulation);
    }

    fn update_spells(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut callbacks = Vec::new();

        for (_, (entity, spell)) in simulation.entities.query_mut::<(&mut Entity, &Spell)>() {
            if let Some(tile) = simulation.field.tile_at_mut((entity.x, entity.y)) {
                tile.set_highlight(spell.requested_highlight);
            }

            if entity.time_frozen || entity.updated || !entity.spawned || entity.deleted {
                continue;
            }

            callbacks.push(entity.update_callback.clone());

            for index in entity.local_components.iter().cloned() {
                let component = simulation.components.get(index).unwrap();

                callbacks.push(component.update_callback.clone());
            }

            entity.updated = true;
        }

        // execute update functions
        for callback in callbacks {
            callback.call(game_io, resources, simulation, ());
        }
    }

    fn execute_attacks(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        for (_, (entity, living)) in simulation.entities.query_mut::<(&Entity, &mut Living)>() {
            if !entity.time_frozen {
                living.hit = false;
            }
        }

        let mut queued_attacks = std::mem::take(&mut simulation.queued_attacks);
        let mut washed_tiles = Vec::new();

        // interactions between attack boxes and tiles
        for attack_box in &queued_attacks {
            let field = &mut simulation.field;
            let Some(tile) = field.tile_at_mut((attack_box.x, attack_box.y)) else {
                continue;
            };

            // resolve wash
            let tile_state = &simulation.tile_states[tile.state_index()];
            let cleanser_element = tile_state.cleanser_element;

            let element = attack_box.props.element;
            let secondary_element = attack_box.props.secondary_element;

            if cleanser_element == Some(element) || cleanser_element == Some(secondary_element) {
                let wash_data = (tile.state_index(), tile.position());

                if !washed_tiles.contains(&wash_data) {
                    washed_tiles.push(wash_data);
                }
            }

            // highlight
            if attack_box.highlight {
                tile.set_highlight(TileHighlight::Solid);
            }

            tile.acknowledge_attacker(attack_box.attacker_id);
        }

        // remove ignored attacks
        for i in (0..queued_attacks.len()).rev() {
            let attack_box = &queued_attacks[i];

            let Some(tile) = simulation.field.tile_at_mut((attack_box.x, attack_box.y)) else {
                continue;
            };

            if tile.ignoring_attacker(attack_box.attacker_id) {
                queued_attacks.swap_remove(i);
            }
        }

        // interactions between attack boxes and entities
        let mut needs_processing = Vec::new();
        let mut all_living_ids = Vec::new();

        for (id, (entity, living)) in simulation.entities.query_mut::<(&Entity, &Living)>() {
            let entity_id = id.into();

            all_living_ids.push(entity_id);

            if !entity.on_field || entity.deleted || !living.hitbox_enabled || living.health <= 0 {
                // entity can't be hit
                continue;
            }

            let mut attack_boxes = Vec::new();

            for attack_box in &queued_attacks {
                if entity.x != attack_box.x || entity.y != attack_box.y {
                    // not on the same tile
                    continue;
                }

                if attack_box.attacker_id == entity_id {
                    // can't hit self
                    continue;
                }

                if attack_box.team == entity.team {
                    // can't hit members of the same team
                    continue;
                }

                attack_boxes.push(attack_box.clone());
            }

            needs_processing.push((
                id,
                living.intangibility.is_enabled(),
                living.defense_rules.clone(),
                attack_boxes,
            ));
        }

        for (id, intangible, defense_rules, attack_boxes) in needs_processing {
            for attack_box in attack_boxes {
                let attacker_id = attack_box.attacker_id;

                // collision callback
                let entities = &mut simulation.entities;
                let Ok(spell) = entities.query_one_mut::<&Spell>(attacker_id.into()) else {
                    continue;
                };

                let collision_callback = spell.collision_callback.clone();
                let attack_callback = spell.attack_callback.clone();

                // defense check against DefenseOrder::Always
                simulation.defense = Defense::new();

                Defense::defend(
                    game_io,
                    resources,
                    simulation,
                    id.into(),
                    &attack_box,
                    &defense_rules,
                    false,
                );

                if simulation.defense.damage_blocked {
                    // blocked by a defense rule such as barrier, mark as ignored as if we collided
                    if let Some(tile) = simulation.field.tile_at_mut((attack_box.x, attack_box.y)) {
                        tile.ignore_attacker(attacker_id);
                    }
                }

                // test tangibility
                if intangible {
                    if let Ok(living) = simulation.entities.query_one_mut::<&mut Living>(id) {
                        if !living.intangibility.try_pierce(&attack_box.props) {
                            continue;
                        }
                    }
                }

                // mark as ignored as we did collide
                if let Some(tile) = simulation.field.tile_at_mut((attack_box.x, attack_box.y)) {
                    tile.ignore_attacker(attacker_id);
                }

                collision_callback.call(game_io, resources, simulation, id.into());

                // defense check against DefenseOrder::CollisionOnly
                Defense::defend(
                    game_io,
                    resources,
                    simulation,
                    id.into(),
                    &attack_box,
                    &defense_rules,
                    true,
                );

                if simulation.defense.damage_blocked {
                    continue;
                }

                if let Ok(living) = simulation.entities.query_one_mut::<&mut Living>(id) {
                    living.queue_hit(attack_box.props);
                }

                // spell attack callback
                attack_callback.call(game_io, resources, simulation, id.into());
            }
        }

        for entity_id in all_living_ids {
            Living::process_hits(game_io, resources, simulation, entity_id);
        }

        Living::aux_prop_cleanup(simulation, |aux_prop| aux_prop.effect().hit_related());

        // resolve wash
        for (state_index, (x, y)) in washed_tiles {
            let Some(tile) = simulation.field.tile_at_mut((x, y)) else {
                continue;
            };

            if tile.state_index() != state_index {
                // already modified
                continue;
            }

            let tile_state = &simulation.tile_states[state_index];
            let request_change = tile_state.can_replace_callback.clone();
            let replace_callback = tile_state.replace_callback.clone();

            if request_change.call(game_io, resources, simulation, (x, y, TileState::NORMAL)) {
                // revert to NORMAL with permission
                if let Some(tile) = simulation.field.tile_at_mut((x, y)) {
                    tile.set_state_index(TileState::NORMAL, None);
                    replace_callback.call(game_io, resources, simulation, (x, y));
                }
            }
        }

        simulation.field.resolve_ignored_attackers();
    }

    fn mark_deleted(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut pending_deletion = Vec::new();

        for (id, living) in simulation.entities.query_mut::<&Living>() {
            if living.max_health == 0 || living.health > 0 {
                continue;
            }

            pending_deletion.push(id);
        }

        for id in pending_deletion {
            Entity::delete(game_io, resources, simulation, id.into());
        }
    }

    fn update_artifacts(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut callbacks = Vec::new();

        for (_, (entity, _artifact)) in simulation.entities.query_mut::<(&mut Entity, &Artifact)>()
        {
            if entity.updated || !entity.spawned || entity.deleted {
                continue;
            }

            callbacks.push(entity.update_callback.clone());

            for index in entity.local_components.iter().cloned() {
                let component = simulation.components.get(index).unwrap();

                callbacks.push(component.update_callback.clone());
            }

            entity.updated = true;
        }

        // execute update functions
        for callback in callbacks {
            callback.call(game_io, resources, simulation, ());
        }
    }

    fn process_input(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        self.process_action_input(game_io, resources, simulation);
        self.process_movement_input(game_io, resources, simulation);
    }

    fn process_action_input(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let status_registry = &resources.status_registry;
        let entities = &mut simulation.entities;

        let player_ids: Vec<_> = entities
            .query_mut::<&Player>()
            .into_iter()
            .map(|(e, _)| e)
            .collect();

        for &id in &player_ids {
            let entities = &mut simulation.entities;
            let entity_id = id.into();

            let (entity, player, living, character) = entities
                .query_one_mut::<(&mut Entity, &mut Player, &Living, &mut Character)>(id)
                .unwrap();

            if !entity.on_field {
                continue;
            }

            let input = &simulation.inputs[player.index];

            // normal attack and charged attack
            if entity.deleted || living.status_director.input_locked_out(status_registry) {
                player.cancel_charge();
                continue;
            }

            let action_processed = (simulation.actions)
                .values()
                .any(|action| action.processed && action.entity == entity_id);

            if action_processed {
                player.cancel_charge();
                continue;
            }

            if character.cards.is_empty() {
                character.card_use_requested = false;
            }

            // ignore other actions if we're waiting on a card
            if character.card_use_requested {
                continue;
            }

            if simulation.time_freeze_tracker.time_is_frozen() {
                // stop adding card actions if time freeze is starting
                // this way time freeze cards aren't eaten
                continue;
            }

            if input.was_just_pressed(Input::Special) {
                Player::use_special_attack(game_io, resources, simulation, id.into());
            } else {
                Player::handle_charging(game_io, resources, simulation, id.into());
            }
        }

        // update charge sprites
        for &id in &player_ids {
            let entities = &mut simulation.entities;

            let (entity, player) = entities
                .query_one_mut::<(&mut Entity, &mut Player)>(id)
                .unwrap();

            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
                player.card_charge.update_sprite(sprite_tree);
                player.attack_charge.update_sprite(sprite_tree);
            }
        }
    }

    fn process_movement_input(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            // shouldn't move during time freeze
            return;
        }

        let entities = &mut simulation.entities;
        let entity_ids: Vec<EntityId> = entities
            .query_mut::<&Player>()
            .into_iter()
            .map(|(id, _)| id.into())
            .collect();

        for entity_id in entity_ids {
            Player::handle_movement_input(game_io, resources, simulation, entity_id);
        }
    }

    fn update_living(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut callbacks = Vec::new();

        let status_registry = &resources.status_registry;
        let entities = &mut simulation.entities;

        type Query<'a> = (
            &'a mut Entity,
            &'a mut Living,
            Option<&'a Player>,
            Option<&'a Movement>,
        );

        for (id, (entity, living, player, movement)) in entities.query_mut::<Query>().into_iter() {
            if entity.updated || !entity.spawned || entity.deleted {
                continue;
            }

            entity.updated = true;

            if living.status_director.is_dragged() && movement.is_none() {
                // let the status director know we're no longer being dragged
                living.status_director.end_drag()
            }

            // process statuses as long as the entity isn't being dragged
            if !living.status_director.is_dragged() {
                let status_director = &mut living.status_director;

                if !entity.time_frozen {
                    // tick statuses only if time isn't frozen
                    status_director.update();
                }

                // apply new statuses even if time is frozen
                status_director.apply_new_statuses(status_registry);

                // status destructors
                callbacks.extend(status_director.take_ready_destructors());

                // new status callbacks
                for hit_flag in status_director.take_new_statuses() {
                    if let Some(callback) = status_registry.status_constructor(hit_flag) {
                        callbacks.push(callback.bind(id.into()));
                    }

                    // call registered status callbacks
                    if let Some(status_callbacks) = living.status_callbacks.get(&hit_flag) {
                        callbacks.extend(status_callbacks.iter().cloned());
                    }
                }
            }

            if entity.time_frozen {
                // skip remaining updates if time is frozen
                continue;
            }

            // update intangibility
            living.intangibility.update();

            let deactivate_callbacks = living.intangibility.take_deactivate_callbacks();
            callbacks.extend(deactivate_callbacks);

            if !living.status_director.is_inactionable(status_registry) {
                callbacks.push(entity.update_callback.clone());

                if let Some(player) = player {
                    if let Some(callback) = player.active_form_update_callback() {
                        callbacks.push(callback.clone());
                    }
                }

                for index in entity.local_components.iter().cloned() {
                    let component = simulation.components.get(index).unwrap();

                    callbacks.push(component.update_callback.clone());
                }
            }
        }

        // execute update functions
        for callback in callbacks {
            callback.call(game_io, resources, simulation, ());
        }
    }

    fn process_movement(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let status_registry = &resources.status_registry;
        let tile_size = simulation.field.tile_size();
        let mut moving_entities = Vec::new();

        for (id, (entity, living, _)) in simulation
            .entities
            .query_mut::<(&Entity, Option<&Living>, &Movement)>()
        {
            let mut update_progress = entity.spawned && !entity.deleted && !entity.time_frozen;

            if let Some(living) = living {
                if living.status_director.is_immobile(status_registry) {
                    update_progress = false;
                }
            }

            moving_entities.push((id, update_progress));
        }

        // movement
        // maybe this should happen in a separate update before everything else updates
        // that way scripts see accurate offsets
        for (id, update_progress) in moving_entities {
            let Ok((mut entity, mut movement)) = simulation
                .entities
                .query_one_mut::<(&mut Entity, &mut Movement)>(id)
            else {
                continue;
            };

            let entity_id = id.into();

            if movement.elapsed == 0 {
                movement.source = (entity.x, entity.y);
            }

            if let Some(callback) = movement.begin_callback.take() {
                simulation.pending_callbacks.push(callback);
            }

            if update_progress {
                movement.elapsed += 1;
            }

            let animation_progress = movement.animation_progress_percent();

            if animation_progress >= 0.5 && !movement.success {
                // test to see if the movement is valid
                let dest = movement.dest;

                if simulation.field.tile_at_mut(dest).is_none()
                    || !Entity::can_move_to(game_io, resources, simulation, id.into(), dest)
                {
                    if let Ok(movement) = simulation.entities.remove_one::<Movement>(id) {
                        simulation.pending_callbacks.extend(movement.end_callback)
                    }

                    continue;
                }

                // refetch entity to satisfy lifetime requirements
                let Ok(tuple) =
                    simulation
                        .entities
                        .query_one_mut::<(&mut Entity, &mut Movement, Option<&ActionQueue>)>(id)
                else {
                    continue;
                };
                (entity, movement, _) = tuple;
                let action_queue = tuple.2;

                // update tile position
                entity.x = dest.0;
                entity.y = dest.1;
                movement.success = true;

                let mut start_tile = simulation.field.tile_at_mut(movement.source);

                if let Some(start_tile) = &mut start_tile {
                    start_tile.handle_auto_reservation_removal(
                        &simulation.actions,
                        entity_id,
                        entity,
                        action_queue,
                    );
                }

                let movement = movement.clone();

                // process stepping off the tile
                if let Some(start_tile) = start_tile {
                    let tile_state = &simulation.tile_states[start_tile.state_index()];
                    let tile_callback = tile_state.entity_leave_callback.clone();
                    let params = (entity_id, movement.source.0, movement.source.1);
                    let callback = tile_callback.bind(params);

                    simulation.pending_callbacks.push(callback);
                }

                // process stepping onto the new tile
                if let Some(dest_tile) = simulation.field.tile_at_mut(movement.dest) {
                    let tile_state = &simulation.tile_states[dest_tile.state_index()];
                    let tile_callback = tile_state.entity_enter_callback.clone();
                    let callback = tile_callback.bind(entity_id);
                    simulation.pending_callbacks.push(callback);
                }

                if let Some(current_tile) = simulation.field.tile_at_mut((entity.x, entity.y)) {
                    current_tile.handle_auto_reservation_addition(
                        &simulation.actions,
                        entity_id,
                        entity,
                        action_queue,
                    );
                }
            }

            if movement.is_complete() {
                if movement.success {
                    if let Some(current_tile) = simulation.field.tile_at_mut((entity.x, entity.y)) {
                        let tile_state = &simulation.tile_states[current_tile.state_index()];
                        let tile_callback = tile_state.entity_stop_callback.clone();
                        let params = (entity_id, movement.source.0, movement.source.1);
                        let callback = tile_callback.bind(params);

                        simulation.pending_callbacks.push(callback);
                    }
                }

                if let Ok(movement) = simulation.entities.remove_one::<Movement>(id) {
                    simulation.pending_callbacks.extend(movement.end_callback)
                }
            } else {
                // apply jump height
                entity.movement_offset.y -= movement.interpolate_jump_height(animation_progress);

                // calculate slide
                let start = IVec2::from(movement.source).as_vec2();
                let dest = IVec2::from(movement.dest).as_vec2();

                let current = if animation_progress < 0.5 {
                    (dest - start) * animation_progress
                } else {
                    (dest - start) * (animation_progress - 1.0)
                };

                entity.movement_offset.x += current.x * tile_size.x;
                entity.movement_offset.y += current.y * tile_size.y;
            }
        }

        simulation.call_pending_callbacks(game_io, resources);
    }

    fn apply_status_vfx(
        &self,
        game_io: &GameIO,
        shared_assets: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let entities = &mut simulation.entities;

        for (_, (entity, living)) in entities.query_mut::<(&mut Entity, &mut Living)>() {
            let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index)
            else {
                continue;
            };

            let status_director = &mut living.status_director;

            if living.hit {
                let root_node = sprite_tree.root_mut();
                root_node.set_color(Color::WHITE);
                root_node.set_color_mode(SpriteColorMode::Add);
            }

            status_director.update_status_sprites(game_io, shared_assets, entity, sprite_tree);
        }
    }
}
