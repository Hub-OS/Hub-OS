use super::State;
use crate::battle::*;
use crate::bindable::*;
use crate::ease::inverse_lerp;
use crate::render::ui::{FontStyle, TextStyle};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use rand::Rng;

#[derive(Clone)]
pub struct BattleState {
    time: FrameTime,
    complete: bool,
    message: Option<(&'static str, FrameTime)>,
}

impl State for BattleState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, game_io: &GameIO) -> Option<Box<dyn State>> {
        if self.complete {
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
        // such as in a BattleStep component or callback

        // execute attacks
        self.execute_attacks(game_io, resources, simulation);

        // process 0 HP
        self.mark_deleted(game_io, resources, simulation);

        // new: update living, processes statuses
        self.update_living(game_io, resources, simulation);

        // update artifacts
        self.update_artifacts(game_io, resources, simulation);

        // update battle step components
        if !simulation.time_freeze_tracker.time_is_frozen() {
            simulation.update_components(game_io, resources, ComponentLifetime::Battle);
        }

        simulation.call_pending_callbacks(game_io, resources);

        self.apply_status_vfx(game_io, resources, simulation);

        if self.message.is_none() && !simulation.time_freeze_tracker.time_is_frozen() {
            // only update the time statistic if the battle is still going for the local player
            // and if time is not frozen
            simulation.statistics.time += 1;
        }

        // other players may still be in battle, and some components make use of this
        simulation.battle_time += 1;
        self.time += 1;

        self.detect_success_or_failure(simulation);
        self.update_turn_gauge(game_io, simulation);
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        // win / lose message
        if let Some((text, start_time)) = self.message {
            const MESSAGE_INTRO_TIME: FrameTime = 10;

            let mut style = TextStyle::new(game_io, FontStyle::Battle);
            style.letter_spacing = 0.0;
            style.scale.y = inverse_lerp!(0, MESSAGE_INTRO_TIME, simulation.time - start_time);

            let size = style.measure(text).size;
            let position = (RESOLUTION_F - size * style.scale) * 0.5;
            style.bounds.set_position(position);

            style.draw(game_io, sprite_queue, text);
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
            complete: false,
            message: None,
        }
    }

    fn update_turn_gauge(&mut self, game_io: &GameIO, simulation: &mut BattleSimulation) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            return;
        }

        let previously_incomplete = !simulation.turn_gauge.is_complete();

        simulation.turn_gauge.increment_time();

        if !simulation.turn_gauge.is_complete() || self.message.is_some() {
            // don't check for input if the battle has ended, or if the turn guage isn't complete
            return;
        }

        if previously_incomplete {
            // just completed, play a sfx
            let globals = game_io.resource::<Globals>().unwrap();
            simulation.play_sound(game_io, &globals.sfx.turn_gauge);
        }

        if simulation.config.turn_limit == Some(simulation.statistics.turns) {
            self.fail(simulation);
            return;
        }

        if simulation.config.automatic_turn_end {
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

        const TOTAL_MESSAGE_TIME: FrameTime = 3 * 60;

        if let Some((_, time)) = self.message {
            if simulation.time - time >= TOTAL_MESSAGE_TIME {
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
                self.fail(simulation);
                return;
            }

            local_team = entity.team;
        } else {
            self.fail(simulation);
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

    fn fail(&mut self, simulation: &BattleSimulation) {
        self.message = Some(("<_FAILED_>", simulation.time));
    }

    fn succeed(&mut self, simulation: &BattleSimulation) {
        self.message = Some(("<_SUCCESS_>", simulation.time));
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
            entity.tile_offset = Vec2::ZERO;

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

        simulation.field.update_tiles(&mut simulation.entities);
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

        let queued_attacks = std::mem::take(&mut simulation.queued_attacks);
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

        // interactions between attack boxes and entities
        let mut needs_processing = Vec::new();
        let mut all_living_ids = Vec::new();

        for (id, (entity, living)) in simulation.entities.query_mut::<(&Entity, &Living)>() {
            all_living_ids.push(id);

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

                if attack_box.attacker_id == entity.id {
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
                simulation.defense_judge = DefenseJudge::new();

                DefenseJudge::judge(
                    game_io,
                    resources,
                    simulation,
                    id.into(),
                    &attack_box,
                    &defense_rules,
                    false,
                );

                if simulation.defense_judge.damage_blocked {
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
                DefenseJudge::judge(
                    game_io,
                    resources,
                    simulation,
                    id.into(),
                    &attack_box,
                    &defense_rules,
                    true,
                );

                if simulation.defense_judge.damage_blocked {
                    continue;
                }

                if let Ok(living) = simulation.entities.query_one_mut::<&mut Living>(id) {
                    living.queue_hit(attack_box.props);
                }

                // spell attack callback
                attack_callback.call(game_io, resources, simulation, id.into());
            }
        }

        for id in all_living_ids {
            Living::process_hits(game_io, resources, simulation, id.into());
        }

        Living::aux_prop_cleanup(simulation, |aux_prop| aux_prop.effect().hit_related());

        // resolve wash
        for (state_index, (x, y)) in washed_tiles {
            let tile = simulation.field.tile_at_mut((x, y)).unwrap();

            if tile.state_index() != state_index {
                // already modified
                continue;
            }

            let tile_state = &simulation.tile_states[state_index];
            let request_change = tile_state.change_request_callback.clone();

            if request_change.call(game_io, resources, simulation, (x, y, TileState::NORMAL)) {
                // revert to NORMAL with permission
                let tile = simulation.field.tile_at_mut((x, y)).unwrap();
                tile.set_state_index(TileState::NORMAL, None);
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

        for (_, (entity, living)) in simulation.entities.query_mut::<(&mut Entity, &Living)>() {
            if living.max_health == 0 || living.health > 0 {
                continue;
            }

            pending_deletion.push(entity.id);
        }

        for id in pending_deletion {
            Entity::delete(game_io, resources, simulation, id);
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
            if simulation.time_freeze_tracker.time_is_frozen() {
                // stop adding card actions if time freeze is starting
                // this way time freeze cards aren't eaten
                break;
            }

            let entities = &mut simulation.entities;

            let (entity, player, living, character) = entities
                .query_one_mut::<(&mut Entity, &mut Player, &Living, &mut Character)>(id)
                .unwrap();

            let input = &simulation.inputs[player.index];

            // normal attack and charged attack
            if entity.deleted || living.status_director.input_locked_out(status_registry) {
                player.cancel_charge();
                continue;
            }

            let action_processed = (simulation.actions)
                .values()
                .any(|action| action.processed && action.entity == entity.id);

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
                player.buster_charge.update_sprite(sprite_tree);
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

        type Query<'a> = (&'a mut Entity, &'a mut Living, Option<&'a Player>);

        for (id, (entity, living, player)) in entities.query_mut::<Query>().into_iter() {
            if entity.time_frozen || entity.updated || !entity.spawned || entity.deleted {
                continue;
            }

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

            entity.updated = true;

            if living.status_director.is_dragged() && entity.movement.is_none() {
                // let the status director know we're no longer being dragged
                living.status_director.end_drag()
            }

            // process statuses as long as the entity isn't being dragged
            if !living.status_director.is_dragged() {
                let status_director = &mut living.status_director;
                status_director.update(status_registry, &simulation.inputs);

                // status destructors
                callbacks.extend(status_director.take_ready_destructors());

                // new status callbacks
                for hit_flag in status_director.take_new_statuses() {
                    if hit_flag & HitFlag::FLASH != HitFlag::NONE {
                        // apply intangible

                        // callback will keep the status director in sync when intangibility is pierced
                        let callback = BattleCallback::new(move |_, _, simulation, _| {
                            let living = simulation
                                .entities
                                .query_one_mut::<&mut Living>(id)
                                .unwrap();

                            living.status_director.remove_status(HitFlag::FLASH)
                        });

                        living.intangibility.enable(IntangibleRule {
                            duration: living.status_director.remaining_status_time(hit_flag),
                            deactivate_callback: Some(callback),
                            ..Default::default()
                        });
                    }

                    if let Some(callback) = status_registry.status_constructor(hit_flag) {
                        callbacks.push(callback.bind(id.into()));
                    }

                    // call registered status callbacks
                    if let Some(status_callbacks) = living.status_callbacks.get(&hit_flag) {
                        callbacks.extend(status_callbacks.iter().cloned());
                    }
                }
            }

            // update intangibility
            living.intangibility.update();

            let deactivate_callbacks = living.intangibility.take_deactivate_callbacks();
            callbacks.extend(deactivate_callbacks);
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

        for (id, entity) in simulation.entities.query::<&Entity>().into_iter() {
            let mut update_progress = entity.spawned && !entity.deleted && !entity.time_frozen;

            if let Some(living) = simulation.entities.query_one::<&Living>(id).unwrap().get() {
                if living.status_director.is_immobile(status_registry) {
                    update_progress = false;
                }
            }

            if entity.movement.is_some() {
                moving_entities.push((id, update_progress));
            }
        }

        // movement
        // maybe this should happen in a separate update before everything else updates
        // that way scripts see accurate offsets
        for (id, update_progress) in moving_entities {
            let mut entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(id)
                .unwrap();

            let Some(movement) = entity.movement.as_mut() else {
                continue;
            };

            if movement.elapsed == 0 {
                movement.source = (entity.x, entity.y);
            }

            if let Some(callback) = movement.on_begin.take() {
                simulation.pending_callbacks.push(callback);
            }

            if update_progress {
                movement.elapsed += 1;
            }

            let animation_progress = movement.animation_progress_percent();

            if animation_progress >= 0.5 && !movement.success {
                // test to see if the movement is valid
                let dest = movement.dest;
                let can_move_to_callback = entity.current_can_move_to_callback(&simulation.actions);

                if simulation.field.tile_at_mut(dest).is_none()
                    || !can_move_to_callback.call(game_io, resources, simulation, dest)
                {
                    let entity = simulation
                        .entities
                        .query_one_mut::<&mut Entity>(id)
                        .unwrap();
                    entity.movement = None;
                    continue;
                }

                // refetch entity to satisfy lifetime requirements
                entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(id)
                    .unwrap();
                let Some(movement) = entity.movement.as_mut() else {
                    continue;
                };

                // update tile position
                entity.x = dest.0;
                entity.y = dest.1;
                movement.success = true;

                let start_tile = simulation.field.tile_at_mut(movement.source).unwrap();
                start_tile.handle_auto_reservation_removal(&simulation.actions, entity);

                if !entity.ignore_negative_tile_effects {
                    let movement = entity.movement.clone().unwrap();

                    // process stepping off the tile
                    let tile_state = &simulation.tile_states[start_tile.state_index()];
                    let tile_callback = tile_state.entity_leave_callback.clone();
                    let params = (entity.id, movement.source.0, movement.source.1);

                    let callback =
                        BattleCallback::new(move |game_io, resources, simulation, ()| {
                            tile_callback.call(game_io, resources, simulation, params);
                        });

                    simulation.pending_callbacks.push(callback);

                    // process stepping onto the new tile
                    let dest_tile = simulation.field.tile_at_mut(movement.dest).unwrap();
                    let tile_state = &simulation.tile_states[dest_tile.state_index()];
                    let tile_callback = tile_state.entity_enter_callback.clone();
                    let params = entity.id;

                    let callback =
                        BattleCallback::new(move |game_io, resources, simulation, ()| {
                            tile_callback.call(game_io, resources, simulation, params);
                        });

                    simulation.pending_callbacks.push(callback);
                }

                let current_tile = simulation.field.tile_at_mut((entity.x, entity.y)).unwrap();
                current_tile.handle_auto_reservation_addition(&simulation.actions, entity);
            }

            let Some(movement) = entity.movement.as_mut() else {
                continue;
            };

            if movement.is_complete() {
                let movement = entity.movement.take().unwrap();

                if movement.success && !entity.ignore_negative_tile_effects {
                    let current_tile = simulation.field.tile_at_mut((entity.x, entity.y)).unwrap();

                    let tile_state = &simulation.tile_states[current_tile.state_index()];
                    let tile_callback = tile_state.entity_stop_callback.clone();

                    let params = (entity.id, movement.source.0, movement.source.1);

                    let callback =
                        BattleCallback::new(move |game_io, resources, simulation, ()| {
                            tile_callback.call(game_io, resources, simulation, params);
                        });

                    simulation.pending_callbacks.push(callback);
                }
            } else {
                // apply jump height
                entity.tile_offset.y -= movement.interpolate_jump_height(animation_progress);

                // calculate slide
                let start = IVec2::from(movement.source).as_vec2();
                let dest = IVec2::from(movement.dest).as_vec2();

                let current = if animation_progress < 0.5 {
                    (dest - start) * animation_progress
                } else {
                    (dest - start) * (animation_progress - 1.0)
                };

                entity.tile_offset.x += current.x * tile_size.x;
                entity.tile_offset.y += current.y * tile_size.y;
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

            if let Some(lifetime) = status_director.status_lifetime(HitFlag::PARALYZE) {
                if (lifetime / 2) % 2 == 0 {
                    let root_sprite = sprite_tree.root_mut();
                    root_sprite.set_color_mode(SpriteColorMode::Add);
                    root_sprite.set_color(Color::YELLOW);
                }
            }

            if let Some(lifetime) = status_director.status_lifetime(HitFlag::ROOT) {
                if (lifetime / 2) % 2 == 0 {
                    let root_sprite = sprite_tree.root_mut();
                    root_sprite.set_color_mode(SpriteColorMode::Multiply);
                    root_sprite.set_color(Color::BLACK);
                }
            }

            if let Some(lifetime) = status_director.status_lifetime(HitFlag::FLASH) {
                if (lifetime / 2) % 2 == 0 {
                    let root_sprite = sprite_tree.root_mut();
                    root_sprite.set_color(Color::TRANSPARENT);
                }
            }

            if status_director.is_shaking() {
                entity.tile_offset.x += simulation.rng.gen_range(-1..=1) as f32;
                status_director.decrement_shake_time();
            }

            if living.hit {
                let root_node = sprite_tree.root_mut();
                root_node.set_color(Color::WHITE);
                root_node.set_color_mode(SpriteColorMode::Add);
            }

            status_director.update_status_sprites(game_io, shared_assets, entity, sprite_tree);
        }
    }
}
